#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <map>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include "zlib.h"
#include "tinyxml.h"
#include "base64.h"
#include "version.h"

using namespace std;

/**************** from tinyxml docs ******************/
// ----------------------------------------------------------------------
// STDOUT dump and indenting utility functions
// ----------------------------------------------------------------------
const unsigned int NUM_INDENTS_PER_SPACE=2;

const char * getIndent( unsigned int numIndents )
{
	static const char * pINDENT="                                      + ";
	static const unsigned int LENGTH=strlen( pINDENT );
	unsigned int n=numIndents*NUM_INDENTS_PER_SPACE;
	if ( n > LENGTH ) n = LENGTH;

	return &pINDENT[ LENGTH-n ];
}

// same as getIndent but no "+" at the end
const char * getIndentAlt( unsigned int numIndents )
{
	static const char * pINDENT="                                        ";
	static const unsigned int LENGTH=strlen( pINDENT );
	unsigned int n=numIndents*NUM_INDENTS_PER_SPACE;
	if ( n > LENGTH ) n = LENGTH;

	return &pINDENT[ LENGTH-n ];
}

int dump_attribs_to_stdout(TiXmlElement* pElement, unsigned int indent)
{
	if ( !pElement ) return 0;

	TiXmlAttribute* pAttrib=pElement->FirstAttribute();
	int i=0;
	int ival;
	double dval;
	const char* pIndent=getIndent(indent);
	printf("\n");
	while (pAttrib)
	{
		printf( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value());

		if (pAttrib->QueryIntValue(&ival)==TIXML_SUCCESS)    printf( " int=%d", ival);
		if (pAttrib->QueryDoubleValue(&dval)==TIXML_SUCCESS) printf( " d=%1.1f", dval);
		printf( "\n" );
		i++;
		pAttrib=pAttrib->Next();
	}
	return i;	
}

void dump_to_stdout( TiXmlNode* pParent, unsigned int indent = 0 )
{
	if ( !pParent ) return;

	TiXmlNode* pChild;
	TiXmlText* pText;
	int t = pParent->Type();
	printf( "%s", getIndent(indent));
	int num;

	switch ( t )
	{
	case TiXmlNode::TINYXML_DOCUMENT:
		printf( "Document" );
		break;

	case TiXmlNode::TINYXML_ELEMENT:
		printf( "Element [%s]", pParent->Value() );
		num=dump_attribs_to_stdout(pParent->ToElement(), indent+1);
		switch(num)
		{
			case 0:  printf( " (No attributes)"); break;
			case 1:  printf( "%s1 attribute", getIndentAlt(indent)); break;
			default: printf( "%s%d attributes", getIndentAlt(indent), num); break;
		}
		break;

	case TiXmlNode::TINYXML_COMMENT:
		printf( "Comment: [%s]", pParent->Value());
		break;

	case TiXmlNode::TINYXML_UNKNOWN:
		printf( "Unknown" );
		break;

	case TiXmlNode::TINYXML_TEXT:
		pText = pParent->ToText();
		printf( "Text: [%s]", pText->Value() );
		break;

	case TiXmlNode::TINYXML_DECLARATION:
		printf( "Declaration" );
		break;
	default:
		break;
	}
	printf( "\n" );
	for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
	{
		dump_to_stdout( pChild, indent+1 );
	}
}

// load the named file and dump its structure to STDOUT
void dump_to_stdout(const char* pFilename)
{
	TiXmlDocument doc(pFilename);
	bool loadOkay = doc.LoadFile();
	if (loadOkay)
	{
		printf("\n%s:\n", pFilename);
		dump_to_stdout( &doc ); // defined later in the tutorial
	}
	else
	{
		printf("Failed to load file \"%s\"\n", pFilename);
	}
}
/**************** from tinyxml docs ******************/

int main(int argc, char **argv)
{
	printf("MAGICK: MathCad Locked Area Password Hash Dumper\nVersion %s; %s build\n", VER_FULLSTR, VER_BUILD_TYPE);
	printf("Build datetime: %s\n", VER_COMPILE_DATETIME);
	printf("Compiled by %s@%s using %s\n", VER_COMPILE_BY, VER_COMPILE_HOST, VER_COMPILER);
	printf("CFLAGS: %s\n", VER_CFLAGS);
	printf("\n");

	if (argc < 2) {
		printf("Syntax: %s filename\n", argv[0]);
		return EXIT_FAILURE;
	}

	printf("Casting magick spells on %s...\n", argv[1]);

	// load the document
	TiXmlDocument doc(argv[1]);
	if (!doc.LoadFile()) {
		printf("Failed to open Mathcad document '%s'.\n", argv[1]);
		return EXIT_FAILURE;
	}

	// dump [debug code]
//	dump_to_stdout(&doc);

	TiXmlElement* pElem;
	TiXmlHandle hDoc(&doc);
	TiXmlHandle hRoot(0);

	// Find the document's root element
	pElem=hDoc.FirstChildElement().Element();

	// should always have a valid root but handle gracefully if it doesn't
	assert(pElem);
//	printf("Root element name: [%s]\n", pElem->Value());
	if (pElem->ValueStr().compare("worksheet") != 0) {
		printf("Root element is not <worksneet>; this is not a Mathcad document.\n");
		return EXIT_FAILURE;
	}
	// save the root handle for later
	hRoot = TiXmlHandle(pElem);

	// now find the regions and iterate over them
	TiXmlElement* pRegions = hRoot.FirstChildElement("regions").Element();
	if (!pRegions) {
		printf("No <regions> block found; this is not a Mathcad document.\n");
		return EXIT_FAILURE;
	}
//	dump_to_stdout(elem);

//	now get an ID list of all the confidentialArea blocks
	vector<int> idlist;
	for (TiXmlNode* pRegion = pRegions->FirstChild(); pRegion != 0; pRegion = pRegion->NextSibling())
	{
		// see if this region has a confidential area attached to it
		TiXmlElement* pCa = pRegion->ToElement()->FirstChildElement("confidentialArea");
		while (pCa) {
//			dump_to_stdout(pCa);

			// get the ID reference of this confidentialArea
			int x;
			if (pCa->QueryIntAttribute("item-idref", &x) != TIXML_SUCCESS) {
				printf("Hmm. A confidentialArea with no item-idref... How strange. Aborting.\n");
				return EXIT_FAILURE;
			} else {
				// got a valid ID, push it into the ID list
				idlist.push_back(x);
			}

			// next element, please!
			pCa = pCa->NextSiblingElement("confidentialArea");
		}
	}

	// ok, we have the id list. find the binaryContent block
	TiXmlElement *pBinaryContent = hRoot.FirstChildElement("binaryContent").Element();
	if (!pBinaryContent) {
		printf("No binary content block found.\n");
		return EXIT_FAILURE;
	}

	// iterate over all the items
	for (TiXmlNode* pArea = pBinaryContent->FirstChild(); pArea != 0; pArea = pArea->NextSibling())
	{
		int itemID;
		if (pArea->ToElement()->QueryIntAttribute("item-id", &itemID) != TIXML_SUCCESS) {
			printf("Seen an item with no item-id attribute...!\n");
			return EXIT_FAILURE;
		}

		if (std::find(idlist.begin(), idlist.end(), itemID) != idlist.end()) {
			printf("item ID %d seen; is in ID List\n", itemID);
			string encoding;

			if (pArea->ToElement()->QueryStringAttribute("content-encoding", &encoding) != TIXML_SUCCESS) {
				encoding = "";
			}

			printf("\tencoding type: [%s]\n", encoding.c_str());
			string b64data(pArea->ToElement()->FirstChild()->ToText()->ValueStr());
			// remove spaces and tabs
			for (string::iterator it = b64data.begin(); it != b64data.end(); it++)
				if ((*it == ' ') || (*it == '\t'))
					b64data.erase(it);

			// break if data block is empty
			if (b64data.length() < 1) {
				printf("Item ID %d contains no Base64 data!\n", itemID);
				return EXIT_FAILURE;
			}

			string data = base64_decode(b64data);
			string decomp_data;

			printf("compressed textlen: %ld\n", data.length());

			if (encoding.compare("gzip") == 0) {
				const size_t BLKSZ = 8192;

				// first need to dump the zlib data to a file
				char tmpfn[32] = "/tmp/magick.XXXXXX";
				int fd = mkstemp(tmpfn);
				if (fd == -1) {
					printf("ERROR! mkstemp failed!\n");
					return EXIT_FAILURE;
				}
				write(fd, data.c_str(), data.length());
				fsync(fd);
				lseek(fd, 0, SEEK_SET);

				// now gunzip it
				gzFile gzf = gzdopen(fd, "rb");
				if (gzf == NULL) {
					close(fd);
					printf("Error GZOPENing our temp file...!\n");
					return EXIT_FAILURE;
				}

				int x;
				char *buf = new char[BLKSZ];
				string str;
				while ((x = gzread(gzf, buf, BLKSZ)) > 0) {
					// deobfuscate before we dump the data in the OP buffer
					for (int n=0; n<x; n++) {
						buf[n] ^= 0xAA;
					}
					str.append(buf, x);
				}
				delete buf;

				// Now reverse the bytes in the string (second level of obfuscation)
				for (string::const_iterator it = str.end()-1; it >= str.begin(); it-=2) {
					decomp_data.push_back(*it);
				}

				printf("decomp data: len %ld, data %s\n", decomp_data.length(), decomp_data.c_str());

				// close the FD, deleting the temp file in the process
				gzclose(gzf);
				close(fd);
			} else {
				// TODO: have never seen this used...!
				printf("NOT_IMPLEMENTED: have never seen a non-gzipped blob! cowardly aborting!\n");
				return EXIT_FAILURE;
				// TODO: we could just do decomp_data = data...
			}

			// We now have the string decomp_data, which contains 
		}
	}

	return EXIT_SUCCESS;
}
