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

std::string depassword(std::string const& encoded_string)
{
	const unsigned char keystream[] = {
		0x2D, 0x91, 0x7C, 0x22, 0x21, 0x4F, 0x2E, 0x9C,
		0x2D, 0xD4, 0x4B, 0x11, 0x58, 0x7E, 0xF6, 0x7F,
		0x8F, 0x2B, 0x2A, 0x4A, 0xAD, 0xD8, 0x22, 0x0B,
		0x52, 0x4E, 0xAB, 0x1F, 0x05, 0xF5, 0x44, 0xDC,
		0x9E, 0xB3, 0x11, 0x6C, 0x70, 0x59, 0xFE, 0xC7,
		0xC1, 0x6B, 0xB7, 0x17, 0xAE, 0xB5, 0x1A, 0xA7,
		0xDF, 0x87, 0xAD, 0x04, 0x67, 0xBD, 0xBA, 0x11,
		0x42, 0x7C, 0x0A, 0x9D, 0x6F, 0x6B
	};
	string output;
	int pos = 0;

	for (string::const_iterator it = encoded_string.begin(); it != encoded_string.end(); it++) {
		char k = (*it ^ keystream[pos++]);
		// deal with Mathcad 14/15 Unicode... by ignoring it <g>
		// FIXME should probably detect MC14/15 and do some proper de-Unicode-ing here
		if (k != 0x00)
			output = output + k;
	}

	return output;
}

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

	printf("Removing area protections from %s...\n", argv[1]);

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
	map<int, TiXmlElement*> idlist;
	for (TiXmlNode* pRegion = pRegions->FirstChild(); pRegion != 0; pRegion = pRegion->NextSibling())
	{
		// see if this region has a confidential area attached to it
		TiXmlElement* pCa = pRegion->ToElement()->FirstChildElement("confidentialArea");
		TiXmlElement* pAr = pRegion->ToElement()->FirstChildElement("area");
		while (pCa) {
//			dump_to_stdout(pCa);

			// get the ID reference of this confidentialArea
			int x;
			if (pCa->QueryIntAttribute("item-idref", &x) != TIXML_SUCCESS) {
				printf("Hmm. A confidentialArea with no item-idref... How strange. Aborting.\n");
				return EXIT_FAILURE;
			} else {
				// got a valid ID, push it into the ID list
				idlist[x] = pRegion->ToElement();
			}

			// next element, please!
			pCa = pCa->NextSiblingElement("confidentialArea");
		}

		// Deal with password-protected areas too (these don't usually have IDRefs)
		while (pAr) {
//			dump_to_stdout(pAr);

			// does this area have a password set?
			string pwhash;
			if (pAr->QueryStringAttribute("password", &pwhash) == TIXML_SUCCESS) {
				printf("Found an unlocked area with a password set --\n");
				string pw = base64_decode(pwhash);
				printf("\tPassword bytes: ");
				for (string::const_iterator it = pw.begin(); it != pw.end(); it++)
					printf("%02X ", static_cast<unsigned char>(*it));
				printf("\n");

				string depw = depassword(pw);
				printf("\tPassword: %s\n", depw.c_str());
			}

			// next area, please!
			pAr = pAr->NextSiblingElement("area");
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

		if (idlist.find(itemID) != idlist.end()) {
			printf("\n");
			printf("Deobfuscating and unlocking area with item-ID %d...\n", itemID);
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

//			printf("compressed textlen: %ld\n", data.length());

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

//				printf("decomp data: len %ld, data %s\n", decomp_data.length(), decomp_data.c_str());

				// close the FD and delete the temp file
				gzclose(gzf);
				close(fd);
				unlink(tmpfn);
			} else {
				// TODO: have never seen this used...!
				printf("NOT_IMPLEMENTED: have never seen a non-gzipped blob! cowardly aborting!\n");
				return EXIT_FAILURE;
				// TODO: we could just do decomp_data = data...
			}

			// We now have the string decomp_data, which contains the Area.
			// Parse and convert it into an XML document.
			TiXmlDocument areaDoc;
			areaDoc.Parse(decomp_data.c_str());

			// Dump the password hash for debugging
			string pwhash = areaDoc.FirstChildElement("area")->Attribute("password");
			printf("\tHashed Password for this area: %s\n", pwhash.c_str());
			string pw = base64_decode(pwhash);
			printf("\tPassword bytes: ");
			for (string::const_iterator it = pw.begin(); it != pw.end(); it++)
				printf("%02X ", static_cast<unsigned char>(*it));
			printf("\n");
			string depw = depassword(pw);
			printf("\tPassword: %s\n", depw.c_str());

			/***
			 * Per CVE-2006-7037:
			 * Complete removal of lock – Inside the Area tag there are is an
			 * ‘is-locked’ attribute. When a lock has been enabled this is set
			 * to true. However to remove the lock all that needs to be done
			 * is change this value to false. Out of completeness the
			 * ‘timestamp’ attribute should be changed to an empty string and
			 * then the ‘password’ attribute removed. Although these last two
			 * changes are not needed to unlock the Area.
			 */
			areaDoc.FirstChildElement("area")->SetAttribute("is-locked", "false");
			areaDoc.FirstChildElement("area")->SetAttribute("timestamp", "");
			areaDoc.FirstChildElement("area")->RemoveAttribute("password");

			// Use the ID map to replace the confidentialArea blocks with good
			// ol' reg'lar Areas, just like Mom used to make.
			idlist[itemID]->ReplaceChild(
					idlist[itemID]->FirstChildElement("confidentialArea"),
					*(areaDoc.FirstChildElement("area"))
					);

			// Finally, remove the binaryContent data chunk for this ID
			pBinaryContent->RemoveChild(pArea);
		}
	}

	printf("\n");

	/***
	 * For bonus points, lets obliterate the document password (har har har)
	 * It turns out this is even easier than "decrypting" (ROFL) the
	 * obfuscated area blocks... Per CVE-2007-4600:
	 *
	 * Once the XML file has been extracted, within the <editor> tag there
	 * will be a <protection> tag. This will look like:
	 *   <protection protection-level="low" password="XZEdIlJPXZxa1CQRKn6Sfw=="/>
	 *
	 * [...]
	 *
	 * Due to these limitations the entire <protection> tag could be removed,
	 * the level of protection could be reduced, or the password could be
	 * changed.
	 *
	 * "When we pull the pin, Mr. Grenade is NOT OUR FRIEND!"
	 */
	do {	// scope limiter
		TiXmlElement *worksheet = doc.FirstChildElement("worksheet");
		TiXmlElement *settings = worksheet->FirstChildElement("settings");
		TiXmlElement *editor = settings ? settings->FirstChildElement("editor") : NULL;
		TiXmlElement *protection = editor ? editor->FirstChildElement("protection") : NULL;

		if (protection) {
			printf("Removing document protection password...\n");
			printf("Protection level is %s\n", protection->Attribute("protection-level"));
			printf("Password hash is %s\n", protection->Attribute("password"));
			// TODO: dump password hash bytes
			editor->RemoveChild(protection);
		} else {
			printf("No document protection password set -- skipping password removal...\n");
		}
	} while (false);	// end scope limiter

	// save our butchered document to a file
	// because after all this hassle... we may as well.
	string fn(argv[1]);
	fn = "unlocked_" + fn;
	printf("\nSaving output to file '%s'...\n", fn.c_str());
	doc.SaveFile(fn.c_str());

	return EXIT_SUCCESS;
}
