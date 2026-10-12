#if defined( _MSC_VER )
	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS		// This test file is not intended to be secure.
	#endif
#endif

#include "tinyxml2.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined( _MSC_VER ) || defined (WIN32)
	#include <crtdbg.h>
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	_CrtMemState startMemState;
	_CrtMemState endMemState;
#else
	#include <sys/stat.h>
	#include <sys/types.h>
#endif

using namespace tinyxml2;
using namespace std;
int gPass = 0;
int gFail = 0;


bool XMLTest (const char* testString, const char* expected, const char* found, bool echo=true, bool extraNL=false )
{
	bool pass;
	if ( !expected && !found )
		pass = true;
	else if ( !expected || !found )
		pass = false;
	else
		pass = !strcmp( expected, found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( !echo ) {
		printf (" %s\n", testString);
	}
	else {
		if ( extraNL ) {
			printf( " %s\n", testString );
			printf( "%s\n", expected );
			printf( "%s\n", found );
		}
		else {
			printf (" %s [%s][%s]\n", testString, expected, found);
		}
	}

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}

bool XMLTest(const char* testString, XMLError expected, XMLError found, bool echo = true, bool extraNL = false)
{
    return XMLTest(testString, XMLDocument::ErrorIDToName(expected), XMLDocument::ErrorIDToName(found), echo, extraNL);
}

bool XMLTest(const char* testString, bool expected, bool found, bool echo = true, bool extraNL = false)
{
    return XMLTest(testString, expected ? "true" : "false", found ? "true" : "false", echo, extraNL);
}

template< class T > bool XMLTest( const char* testString, T expected, T found, bool echo=true )
{
	bool pass = ( expected == found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( !echo )
		printf (" %s\n", testString);
	else {
		char expectedAsString[64];
		XMLUtil::ToStr(expected, expectedAsString, sizeof(expectedAsString));

		char foundAsString[64];
		XMLUtil::ToStr(found, foundAsString, sizeof(foundAsString));

		printf (" %s [%s][%s]\n", testString, expectedAsString, foundAsString );
	}

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}


void NullLineEndings( char* p )
{
	while( p && *p ) {
		if ( *p == '\n' || *p == '\r' ) {
			*p = 0;
			return;
		}
		++p;
	}
}


int example_1()
{
	XMLDocument doc;
	doc.LoadFile( "resources/dream.xml" );

	return doc.ErrorID();
}
/** @page Example_1 Load an XML File
 *  @dontinclude ./xmltest.cpp
 *  Basic XML file loading.
 *  The basic syntax to load an XML file from
 *  disk and check for an error. (ErrorID()
 *  will return 0 for no error.)
 *  @skip example_1()
 *  @until }
 */


int example_2()
{
	static const char* xml = "<element/>";
	XMLDocument doc;
	doc.Parse( xml );

	return doc.ErrorID();
}
/** @page Example_2 Parse an XML from char buffer
 *  @dontinclude ./xmltest.cpp
 *  Basic XML string parsing.
 *  The basic syntax to parse an XML for
 *  a char* and check for an error. (ErrorID()
 *  will return 0 for no error.)
 *  @skip example_2()
 *  @until }
 */


int example_3()
{
	static const char* xml =
		"<?xml version=\"1.0\"?>"
		"<!DOCTYPE PLAY SYSTEM \"play.dtd\">"
		"<PLAY>"
		"<TITLE>A Midsummer Night's Dream</TITLE>"
		"</PLAY>";

	XMLDocument doc;
	doc.Parse( xml );

	XMLElement* titleElement = doc.FirstChildElement( "PLAY" )->FirstChildElement( "TITLE" );
	const char* title = titleElement->GetText();
	printf( "Name of play (1): %s\n", title );

	XMLText* textNode = titleElement->FirstChild()->ToText();
	title = textNode->Value();
	printf( "Name of play (2): %s\n", title );

	return doc.ErrorID();
}
/** @page Example_3 Get information out of XML
	@dontinclude ./xmltest.cpp
	In this example, we navigate a simple XML
	file, and read some interesting text. Note
	that this example doesn't use error
	checking; working code should check for null
	pointers when walking an XML tree, or use
	XMLHandle.

	(The XML is an excerpt from "dream.xml").

	@skip example_3()
	@until </PLAY>";

	The structure of the XML file is:

	<ul>
		<li>(declaration)</li>
		<li>(dtd stuff)</li>
		<li>Element "PLAY"</li>
		<ul>
			<li>Element "TITLE"</li>
			<ul>
			    <li>Text "A Midsummer Night's Dream"</li>
			</ul>
		</ul>
	</ul>

	For this example, we want to print out the
	title of the play. The text of the title (what
	we want) is child of the "TITLE" element which
	is a child of the "PLAY" element.

	We want to skip the declaration and dtd, so the
	method FirstChildElement() is a good choice. The
	FirstChildElement() of the Document is the "PLAY"
	Element, the FirstChildElement() of the "PLAY" Element
	is the "TITLE" Element.

	@until ( "TITLE" );

	We can then use the convenience function GetText()
	to get the title of the play.

	@until title );

	Text is just another Node in the XML DOM. And in
	fact you should be a little cautious with it, as
	text nodes can contain elements.

	@verbatim
	Consider: A Midsummer Night's <b>Dream</b>
	@endverbatim

	It is more correct to actually query the Text Node
	if in doubt:

	@until title );

	Noting that here we use FirstChild() since we are
	looking for XMLText, not an element, and ToText()
	is a cast from a Node to a XMLText.
*/


bool example_4()
{
	static const char* xml =
		"<information>"
		"	<attributeApproach v='2' />"
		"	<textApproach>"
		"		<v>2</v>"
		"	</textApproach>"
		"</information>";

	XMLDocument doc;
	doc.Parse( xml );

	int v0 = 0;
	int v1 = 0;

	XMLElement* attributeApproachElement = doc.FirstChildElement()->FirstChildElement( "attributeApproach" );
	attributeApproachElement->QueryIntAttribute( "v", &v0 );

	XMLElement* textApproachElement = doc.FirstChildElement()->FirstChildElement( "textApproach" );
	textApproachElement->FirstChildElement( "v" )->QueryIntText( &v1 );

	printf( "Both values are the same: %d and %d\n", v0, v1 );

	return !doc.Error() && ( v0 == v1 );
}
/** @page Example_4 Read attributes and text information.
	@dontinclude ./xmltest.cpp

	There are fundamentally 2 ways of writing a key-value
	pair into an XML file. (Something that's always annoyed
	me about XML.) Either by using attributes, or by writing
	the key name into an element and the value into
	the text node wrapped by the element. Both approaches
	are illustrated in this example, which shows two ways
	to encode the value "2" into the key "v":

	@skip example_4()
	@until "</information>";

	TinyXML-2 has accessors for both approaches.

	When using an attribute, you navigate to the XMLElement
	with that attribute and use the QueryIntAttribute()
	group of methods. (Also QueryFloatAttribute(), etc.)

	@skip XMLElement* attributeApproachElement
	@until &v0 );

	When using the text approach, you need to navigate
	down one more step to the XMLElement that contains
	the text. Note the extra FirstChildElement( "v" )
	in the code below. The value of the text can then
	be safely queried with the QueryIntText() group
	of methods. (Also QueryFloatText(), etc.)

	@skip XMLElement* textApproachElement
	@until &v1 );
*/


int main( int argc, const char ** argv )
{
	#if defined( _MSC_VER ) && defined( TINYXML2_DEBUG )
		_CrtMemCheckpoint( &startMemState );
		// Enable MS Visual C++ debug heap memory leaks dump on exit
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
		{
			int leaksOnStart = _CrtDumpMemoryLeaks();
			XMLTest( "No leaks on start?", FALSE, leaksOnStart );
		}
	#endif

	{
		TIXMLASSERT( true );
	}

	if ( argc > 1 ) {
		XMLDocument* doc = new XMLDocument();
		clock_t startTime = clock();
		doc->LoadFile( argv[1] );
 		clock_t loadTime = clock();
		int errorID = doc->ErrorID();
		delete doc; doc = 0;
 		clock_t deleteTime = clock();

		printf( "Test file '%s' loaded. ErrorID=%d\n", argv[1], errorID );
		if ( !errorID ) {
			printf( "Load time=%u\n",   (unsigned)(loadTime - startTime) );
			printf( "Delete time=%u\n", (unsigned)(deleteTime - loadTime) );
			printf( "Total time=%u\n",  (unsigned)(deleteTime - startTime) );
		}
		exit(0);
	}

	FILE* fp = fopen( "resources/dream.xml", "r" );
	if ( !fp ) {
		printf( "Error opening test file 'dream.xml'.\n"
				"Is your working directory the same as where \n"
				"the xmltest.cpp and dream.xml file are?\n\n"
	#if defined( _MSC_VER )
				"In windows Visual Studio you may need to set\n"
				"Properties->Debugging->Working Directory to '..'\n"
	#endif
			  );
		exit( 1 );
	}
	fclose( fp );

	XMLTest( "Example_1", 0, example_1() );
	XMLTest( "Example_2", 0, example_2() );
	XMLTest( "Example_3", 0, example_3() );
	XMLTest( "Example_4", true, example_4() );

	/* ------ Example 2: Lookup information. ---- */

	{
		static const char* test[] = {	"<element />",
										"<element></element>",
										"<element><subelement/></element>",
										"<element><subelement></subelement></element>",
										"<element><subelement><subsub/></subelement></element>",
										"<!--comment beside elements--><element><subelement></subelement></element>",
										"<!--comment beside elements, this time with spaces-->  \n <element>  <subelement> \n </subelement> </element>",
										"<element attrib1='foo' attrib2=\"bar\" ></element>",
										"<element attrib1='foo' attrib2=\"bar\" ><subelement attrib3='yeehaa' /></element>",
										"<element>Text inside element.</element>",
										"<element><b></b></element>",
										"<element>Text inside and <b>bolded</b> in the element.</element>",
										"<outer><element>Text inside and <b>bolded</b> in the element.</element></outer>",
										"<element>This &amp; That.</element>",
										"<element attrib='This&lt;That' />",
										0
		};
		for( int i=0; test[i]; ++i ) {
			XMLDocument doc;
			doc.Parse( test[i] );
			XMLTest( "Element test", false, doc.Error() );
			doc.Print();
			printf( "----------------------------------------------\n" );
		}
	}
#if 1
	{
		static const char* test = "<!--hello world\n"
								  "          line 2\r"
								  "          line 3\r\n"
								  "          line 4\n\r"
								  "          line 5\r-->";

		XMLDocument doc;
		doc.Parse( test );
		XMLTest( "Hello world declaration", false, doc.Error() );
		doc.Print();
	}

	{
		// This test is pre-test for the next one
		// (where Element1 is inserted "after itself".
		// This code didn't use to crash.
		XMLDocument doc;
		XMLElement* element1 = doc.NewElement("Element1");
		XMLElement* element2 = doc.NewElement("Element2");
		doc.InsertEndChild(element1);
		doc.InsertEndChild(element2);
		doc.InsertAfterChild(element2, element2);
		doc.InsertAfterChild(element2, element2);
	}

	{
		XMLDocument doc;
		XMLElement* element1 = doc.NewElement("Element1");
		XMLElement* element2 = doc.NewElement("Element2");
		doc.InsertEndChild(element1);
		doc.InsertEndChild(element2);

		// This insertion "after itself"
		// used to cause invalid memory access and crash
		doc.InsertAfterChild(element1, element1);
		doc.InsertAfterChild(element1, element1);
		doc.InsertAfterChild(element2, element2);
		doc.InsertAfterChild(element2, element2);
	}

	{
		static const char* test = "<element>Text before.</element>";
		XMLDocument doc;
		doc.Parse( test );
		XMLTest( "Element text before", false, doc.Error() );
		XMLElement* root = doc.FirstChildElement();
		XMLElement* newElement = doc.NewElement( "Subelement" );
		root->InsertEndChild( newElement );
		doc.Print();
	}
	{
		XMLDocument* doc = new XMLDocument();
		static const char* test = "<element><sub/></element>";
		doc->Parse( test );
		XMLTest( "Element with sub element", false, doc->Error() );
		delete doc;
	}
	{
		// Test: Programmatic DOM nodes insertion return values
		XMLDocument doc;

		XMLNode* first = doc.NewElement( "firstElement" );
		XMLTest( "New element", true, first != 0 );
		XMLNode* firstAfterInsertion = doc.InsertFirstChild( first );
		XMLTest( "New element inserted first", true, firstAfterInsertion == first );

		XMLNode* last = doc.NewElement( "lastElement" );
		XMLTest( "New element", true, last != 0 );
		XMLNode* lastAfterInsertion = doc.InsertEndChild( last );
		XMLTest( "New element inserted last", true, lastAfterInsertion == last );

		XMLNode* middle = doc.NewElement( "middleElement" );
		XMLTest( "New element", true, middle != 0 );
		XMLNode* middleAfterInsertion = doc.InsertAfterChild( first, middle );
		XMLTest( "New element inserted middle", true, middleAfterInsertion == middle );
	}
	{
		// Test: Programmatic DOM
		// Build:
		//		<element>
		//			<!--comment-->
		//			<sub attrib="0" />
		//			<sub attrib="1" />
		//			<sub attrib="2" >& Text!</sub>
		//		<element>

		XMLDocument* doc = new XMLDocument();
		XMLNode* element = doc->InsertEndChild( doc->NewElement( "element" ) );

		XMLElement* sub[3] = { doc->NewElement( "sub" ), doc->NewElement( "sub" ), doc->NewElement( "sub" ) };
		for( int i=0; i<3; ++i ) {
			sub[i]->SetAttribute( "attrib", i );
		}
		element->InsertEndChild( sub[2] );

		const int dummyInitialValue = 1000;
		int dummyValue = dummyInitialValue;

		XMLNode* comment = element->InsertFirstChild( doc->NewComment( "comment" ) );
		comment->SetUserData(&dummyValue);
		element->InsertAfterChild( comment, sub[0] );
		element->InsertAfterChild( sub[0], sub[1] );
		sub[2]->InsertFirstChild( doc->NewText( "& Text!" ));
		doc->Print();
		XMLTest( "Programmatic DOM", "comment", doc->FirstChildElement( "element" )->FirstChild()->Value() );
		XMLTest( "Programmatic DOM", "0", doc->FirstChildElement( "element" )->FirstChildElement()->Attribute( "attrib" ) );
		XMLTest( "Programmatic DOM", 2, doc->FirstChildElement()->LastChildElement( "sub" )->IntAttribute( "attrib" ) );
		XMLTest( "Programmatic DOM", "& Text!",
				 doc->FirstChildElement()->LastChildElement( "sub" )->FirstChild()->ToText()->Value() );
		XMLTest("User data - pointer", true, &dummyValue == comment->GetUserData(), false);
		XMLTest("User data - value behind pointer", dummyInitialValue, dummyValue, false);

		// And now deletion:
		element->DeleteChild( sub[2] );
		doc->DeleteNode( comment );

		element->FirstChildElement()->SetAttribute( "attrib", true );
		element->LastChildElement()->DeleteAttribute( "attrib" );

		XMLTest( "Programmatic DOM", true, doc->FirstChildElement()->FirstChildElement()->BoolAttribute( "attrib" ) );
		const int defaultIntValue = 10;
		const int replacementIntValue = 20;
		int value1 = defaultIntValue;
		int value2 = doc->FirstChildElement()->LastChildElement()->IntAttribute( "attrib", replacementIntValue );
		XMLError result = doc->FirstChildElement()->LastChildElement()->QueryIntAttribute( "attrib", &value1 );
		XMLTest( "Programmatic DOM", XML_NO_ATTRIBUTE, result );
		XMLTest( "Programmatic DOM", defaultIntValue, value1 );
		XMLTest( "Programmatic DOM", replacementIntValue, value2 );

		doc->Print();

		{
			XMLPrinter streamer;
			doc->Print( &streamer );
			printf( "%s", streamer.CStr() );
		}
		{
			XMLPrinter streamer( 0, true );
			doc->Print( &streamer );
			XMLTest( "Compact mode", "<element><sub attrib=\"true\"/><sub/></element>", streamer.CStr(), false );
		}
		doc->SaveFile( "./resources/out/pretty.xml" );
		XMLTest( "Save pretty.xml", false, doc->Error() );
		doc->SaveFile( "./resources/out/compact.xml", true );
		XMLTest( "Save compact.xml", false, doc->Error() );
		delete doc;
	}
	{
		// Test: Dream
		// XML1 : 1,187,569 bytes	in 31,209 allocations
		// XML2 :   469,073	bytes	in    323 allocations
		//int newStart = gNew;
		XMLDocument doc;
		doc.LoadFile( "resources/dream.xml" );
		XMLTest( "Load dream.xml", false, doc.Error() );

		doc.SaveFile( "resources/out/dreamout.xml" );
		XMLTest( "Save dreamout.xml", false, doc.Error() );
		doc.PrintError();

		XMLTest( "Dream", "xml version=\"1.0\"",
						  doc.FirstChild()->ToDeclaration()->Value() );
		XMLTest( "Dream", true, doc.FirstChild()->NextSibling()->ToUnknown() != 0 );
		XMLTest( "Dream", "DOCTYPE PLAY SYSTEM \"play.dtd\"",
						  doc.FirstChild()->NextSibling()->ToUnknown()->Value() );
		XMLTest( "Dream", "And Robin shall restore amends.",
						  doc.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );
		XMLTest( "Dream", "And Robin shall restore amends.",
						  doc.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );

		XMLDocument doc2;
		doc2.LoadFile( "resources/out/dreamout.xml" );
		XMLTest( "Load dreamout.xml", false, doc2.Error() );
		XMLTest( "Dream-out", "xml version=\"1.0\"",
						  doc2.FirstChild()->ToDeclaration()->Value() );
		XMLTest( "Dream-out", true, doc2.FirstChild()->NextSibling()->ToUnknown() != 0 );
		XMLTest( "Dream-out", "DOCTYPE PLAY SYSTEM \"play.dtd\"",
						  doc2.FirstChild()->NextSibling()->ToUnknown()->Value() );
		XMLTest( "Dream-out", "And Robin shall restore amends.",
						  doc2.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );

		//gNewTotal = gNew - newStart;
	}


	{
		const char* error =	"<?xml version=\"1.0\" standalone=\"no\" ?>\n"
							"<passages count=\"006\" formatversion=\"20020620\">\n"
							"    <wrong error>\n"
							"</passages>";

		XMLDocument doc;
		doc.Parse( error );
		XMLTest( "Bad XML", XML_ERROR_PARSING_ATTRIBUTE, doc.ErrorID() );
		const char* errorStr = doc.ErrorStr();
		XMLTest("Formatted error string",
			"Error=XML_ERROR_PARSING_ATTRIBUTE ErrorID=7 (0x7) Line number=3: XMLElement name=wrong",
			errorStr);
	}

	{
		const char* str = "<doc attr0='1' attr1='2.0' attr2='foo' />";

		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "Top level attributes", false, doc.Error() );

		XMLElement* ele = doc.FirstChildElement();

		int iVal;
		XMLError result;
		double dVal;

		result = ele->QueryDoubleAttribute( "attr0", &dVal );
		XMLTest( "Query attribute: int as double", XML_SUCCESS, result);
		XMLTest( "Query attribute: int as double", 1, (int)dVal );
		XMLTest( "Query attribute: int as double", 1, (int)ele->DoubleAttribute("attr0"));

		result = ele->QueryDoubleAttribute( "attr1", &dVal );
		XMLTest( "Query attribute: double as double", XML_SUCCESS, result);
		XMLTest( "Query attribute: double as double", 2.0, dVal );
		XMLTest( "Query attribute: double as double", 2.0, ele->DoubleAttribute("attr1") );

		result = ele->QueryIntAttribute( "attr1", &iVal );
		XMLTest( "Query attribute: double as int", XML_SUCCESS, result);
		XMLTest( "Query attribute: double as int", 2, iVal );

		result = ele->QueryIntAttribute( "attr2", &iVal );
		XMLTest( "Query attribute: not a number", XML_WRONG_ATTRIBUTE_TYPE, result );
		XMLTest( "Query attribute: not a number", 4.0, ele->DoubleAttribute("attr2", 4.0) );

		result = ele->QueryIntAttribute( "bar", &iVal );
		XMLTest( "Query attribute: does not exist", XML_NO_ATTRIBUTE, result );
		XMLTest( "Query attribute: does not exist", true, ele->BoolAttribute("bar", true) );
	}

	{
		const char* str = "<doc/>";

		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "Empty top element", false, doc.Error() );

		XMLElement* ele = doc.FirstChildElement();

		int iVal, iVal2;
		double dVal, dVal2;

		ele->SetAttribute( "str", "strValue" );
		ele->SetAttribute( "int", 1 );
		ele->SetAttribute( "double", -1.0 );

		const char* answer = 0;
		ele->QueryAttribute("str", &answer);
		XMLTest("Query char attribute", "strValue", answer);

		const char* cStr = ele->Attribute( "str" );
		{
			XMLError queryResult = ele->QueryIntAttribute( "int", &iVal );
			XMLTest( "Query int attribute", XML_SUCCESS, queryResult);
		}
		{
			XMLError queryResult = ele->QueryDoubleAttribute( "double", &dVal );
			XMLTest( "Query double attribute", XML_SUCCESS, queryResult);
		}

		{
			XMLError queryResult = ele->QueryAttribute( "int", &iVal2 );
			XMLTest( "Query int attribute generic", (int)XML_SUCCESS, queryResult);
		}
		{
			XMLError queryResult = ele->QueryAttribute( "double", &dVal2 );
			XMLTest( "Query double attribute generic", (int)XML_SUCCESS, queryResult);
		}

		XMLTest( "Attribute match test", "strValue", ele->Attribute( "str", "strValue" ) );
		XMLTest( "Attribute round trip. c-string.", "strValue", cStr );
		XMLTest( "Attribute round trip. int.", 1, iVal );
		XMLTest( "Attribute round trip. double.", -1, (int)dVal );
		XMLTest( "Alternate query", true, iVal == iVal2 );
		XMLTest( "Alternate query", true, dVal == dVal2 );
		XMLTest( "Alternate query", true, iVal == ele->IntAttribute("int") );
		XMLTest( "Alternate query", true, dVal == ele->DoubleAttribute("double") );
	}

	{
		XMLDocument doc;
		doc.LoadFile( "resources/utf8test.xml" );
		XMLTest( "Load utf8test.xml", false, doc.Error() );

		// Get the attribute "value" from the "Russian" element and check it.
		XMLElement* element = doc.FirstChildElement( "document" )->FirstChildElement( "Russian" );
		const unsigned char correctValue[] = {	0xd1U, 0x86U, 0xd0U, 0xb5U, 0xd0U, 0xbdU, 0xd0U, 0xbdU,
												0xd0U, 0xbeU, 0xd1U, 0x81U, 0xd1U, 0x82U, 0xd1U, 0x8cU, 0 };

		XMLTest( "UTF-8: Russian value.", (const char*)correctValue, element->Attribute( "value" ) );

		const unsigned char russianElementName[] = {	0xd0U, 0xa0U, 0xd1U, 0x83U,
														0xd1U, 0x81U, 0xd1U, 0x81U,
														0xd0U, 0xbaU, 0xd0U, 0xb8U,
														0xd0U, 0xb9U, 0 };
		const char russianText[] = "<\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xB5\xD1\x82>";

		XMLText* text = doc.FirstChildElement( "document" )->FirstChildElement( (const char*) russianElementName )->FirstChild()->ToText();
		XMLTest( "UTF-8: Browsing russian element name.",
				 russianText,
				 text->Value() );

		// Now try for a round trip.
		doc.SaveFile( "resources/out/utf8testout.xml" );
		XMLTest( "UTF-8: Save testout.xml", false, doc.Error() );

		// Check the round trip.
		bool roundTripOkay = false;

		FILE* saved  = fopen( "resources/out/utf8testout.xml", "r" );
		XMLTest( "UTF-8: Open utf8testout.xml", true, saved != 0 );

		FILE* verify = fopen( "resources/utf8testverify.xml", "r" );
		XMLTest( "UTF-8: Open utf8testverify.xml", true, verify != 0 );

		if ( saved && verify )
		{
			roundTripOkay = true;
			char verifyBuf[256];
			while ( fgets( verifyBuf, 256, verify ) )
			{
				char savedBuf[256];
				fgets( savedBuf, 256, saved );
				NullLineEndings( verifyBuf );
				NullLineEndings( savedBuf );

				if ( strcmp( verifyBuf, savedBuf ) )
				{
					printf( "verify:%s<\n", verifyBuf );
					printf( "saved :%s<\n", savedBuf );
					roundTripOkay = false;
					break;
				}
			}
		}
		if ( saved )
			fclose( saved );
		if ( verify )
			fclose( verify );
		XMLTest( "UTF-8: Verified multi-language round trip.", true, roundTripOkay );
	}

	// --------GetText()-----------
	{
		const char* str = "<foo>This is  text</foo>";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "Double whitespace", false, doc.Error() );
		const XMLElement* element = doc.RootElement();

		XMLTest( "GetText() normal use.", "This is  text", element->GetText() );

		str = "<foo><b>This is text</b></foo>";
		doc.Parse( str );
		XMLTest( "Bold text simulation", false, doc.Error() );
		element = doc.RootElement();

		XMLTest( "GetText() contained element.", element->GetText() == 0, true );
	}


	// --------SetText()-----------
	{
		const char* str = "<foo></foo>";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "Empty closed element", false, doc.Error() );
		XMLElement* element = doc.RootElement();

		element->SetText("darkness.");
		XMLTest( "SetText() normal use (open/close).", "darkness.", element->GetText() );

		element->SetText("blue flame.");
		XMLTest( "SetText() replace.", "blue flame.", element->GetText() );

		str = "<foo/>";
		doc.Parse( str );
		XMLTest( "Empty self-closed element", false, doc.Error() );
		element = doc.RootElement();

		element->SetText("The driver");
		XMLTest( "SetText() normal use. (self-closing)", "The driver", element->GetText() );

		element->SetText("<b>horses</b>");
		XMLTest( "SetText() replace with tag-like text.", "<b>horses</b>", element->GetText() );
		//doc.Print();

		str = "<foo><bar>Text in nested element</bar></foo>";
		doc.Parse( str );
		XMLTest( "Text in nested element", false, doc.Error() );
		element = doc.RootElement();

		element->SetText("wolves");
		XMLTest( "SetText() prefix to nested non-text children.", "wolves", element->GetText() );

		str = "<foo/>";
		doc.Parse( str );
		XMLTest( "Empty self-closed element round 2", false, doc.Error() );
		element = doc.RootElement();

		element->SetText( "str" );
		XMLTest( "SetText types", "str", element->GetText() );

		element->SetText( 1 );
		XMLTest( "SetText types", "1", element->GetText() );

		element->SetText( 1U );
		XMLTest( "SetText types", "1", element->GetText() );

		element->SetText( true );
		XMLTest( "SetText types", "true", element->GetText() );

		element->SetText( 1.5f );
		XMLTest( "SetText types", "1.5", element->GetText() );

		element->SetText( 1.5 );
		XMLTest( "SetText types", "1.5", element->GetText() );
	}

	// ---------- Attributes ---------
	{
		static const int64_t BIG = -123456789012345678;
        static const uint64_t BIG_POS = 123456789012345678;
		XMLDocument doc;
		XMLElement* element = doc.NewElement("element");
		doc.InsertFirstChild(element);

		{
			element->SetAttribute("attrib", int(-100));
			{
				int v = 0;
				XMLError queryResult = element->QueryIntAttribute("attrib", &v);
				XMLTest("Attribute: int", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: int", -100, v, true);
			}
			{
				int v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: int", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: int", -100, v, true);
			}
			XMLTest("Attribute: int", -100, element->IntAttribute("attrib"), true);
		}
		{
			element->SetAttribute("attrib", unsigned(100));
			{
				unsigned v = 0;
				XMLError queryResult = element->QueryUnsignedAttribute("attrib", &v);
				XMLTest("Attribute: unsigned", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: unsigned", unsigned(100), v, true);
			}
			{
				unsigned v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: unsigned", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: unsigned", unsigned(100), v, true);
			}
			{
				const char* v = "failed";
				XMLError queryResult = element->QueryStringAttribute("not-attrib", &v);
				XMLTest("Attribute: string default", false, queryResult == XML_SUCCESS);
				queryResult = element->QueryStringAttribute("attrib", &v);
				XMLTest("Attribute: string", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: string", "100", v);
			}
			XMLTest("Attribute: unsigned", unsigned(100), element->UnsignedAttribute("attrib"), true);
		}
		{
			element->SetAttribute("attrib", BIG);
			{
				int64_t v = 0;
				XMLError queryResult = element->QueryInt64Attribute("attrib", &v);
				XMLTest("Attribute: int64_t", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: int64_t", BIG, v, true);
			}
			{
				int64_t v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: int64_t", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: int64_t", BIG, v, true);
			}
			XMLTest("Attribute: int64_t", BIG, element->Int64Attribute("attrib"), true);
		}
        {
            element->SetAttribute("attrib", BIG_POS);
            {
                uint64_t v = 0;
                XMLError queryResult = element->QueryUnsigned64Attribute("attrib", &v);
                XMLTest("Attribute: uint64_t", XML_SUCCESS, queryResult, true);
                XMLTest("Attribute: uint64_t", BIG_POS, v, true);
            }
            {
                uint64_t v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
                XMLTest("Attribute: uint64_t", (int)XML_SUCCESS, queryResult, true);
                XMLTest("Attribute: uint64_t", BIG_POS, v, true);
            }
            XMLTest("Attribute: uint64_t", BIG_POS, element->Unsigned64Attribute("attrib"), true);
        }
        {
			element->SetAttribute("attrib", true);
			{
				bool v = false;
				XMLError queryResult = element->QueryBoolAttribute("attrib", &v);
				XMLTest("Attribute: bool", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: bool", true, v, true);
			}
			{
				bool v = false;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: bool", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: bool", true, v, true);
			}
			XMLTest("Attribute: bool", true, element->BoolAttribute("attrib"), true);
		}
		{
			element->SetAttribute("attrib", true);
			const char* result = element->Attribute("attrib");
			XMLTest("Bool true is 'true'", "true", result);

			XMLUtil::SetBoolSerialization("1", "0");
			element->SetAttribute("attrib", true);
			result = element->Attribute("attrib");
			XMLTest("Bool true is '1'", "1", result);

			XMLUtil::SetBoolSerialization(0, 0);
		}
		{
			element->SetAttribute("attrib", 100.0);
			{
				double v = 0;
				XMLError queryResult = element->QueryDoubleAttribute("attrib", &v);
				XMLTest("Attribute: double", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: double", 100.0, v, true);
			}
			{
				double v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: bool", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: double", 100.0, v, true);
			}
			XMLTest("Attribute: double", 100.0, element->DoubleAttribute("attrib"), true);
		}
		{
			element->SetAttribute("attrib", 100.0f);
			{
				float v = 0;
				XMLError queryResult = element->QueryFloatAttribute("attrib", &v);
				XMLTest("Attribute: float", XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: float", 100.0f, v, true);
			}
			{
				float v = 0;
				XMLError queryResult = element->QueryAttribute("attrib", &v);
				XMLTest("Attribute: float", (int)XML_SUCCESS, queryResult, true);
				XMLTest("Attribute: float", 100.0f, v, true);
			}
			XMLTest("Attribute: float", 100.0f, element->FloatAttribute("attrib"), true);
		}
		{
			element->SetText(BIG);
			int64_t v = 0;
			XMLError queryResult = element->QueryInt64Text(&v);
			XMLTest("Element: int64_t", XML_SUCCESS, queryResult, true);
			XMLTest("Element: int64_t", BIG, v, true);
		}
        {
            element->SetText(BIG_POS);
            uint64_t v = 0;
            XMLError queryResult = element->QueryUnsigned64Text(&v);
            XMLTest("Element: uint64_t", XML_SUCCESS, queryResult, true);
            XMLTest("Element: uint64_t", BIG_POS, v, true);
        }
    }

	// ---------- XMLPrinter stream mode ------
	{
		{
			FILE* printerfp = fopen("resources/out/printer.xml", "w");
			XMLTest("Open printer.xml", true, printerfp != 0);
			XMLPrinter printer(printerfp);
			printer.OpenElement("foo");
			printer.PushAttribute("attrib-text", "text");
			printer.PushAttribute("attrib-int", int(1));
			printer.PushAttribute("attrib-unsigned", unsigned(2));
			printer.PushAttribute("attrib-int64", int64_t(3));
			printer.PushAttribute("attrib-uint64", uint64_t(37));
			printer.PushAttribute("attrib-bool", true);
			printer.PushAttribute("attrib-double", 4.0);
			printer.CloseElement();
			fclose(printerfp);
		}
		{
			XMLDocument doc;
			doc.LoadFile("resources/out/printer.xml");
			XMLTest("XMLPrinter Stream mode: load", XML_SUCCESS, doc.ErrorID(), true);

			const XMLDocument& cdoc = doc;

			const XMLAttribute* attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-text");
			XMLTest("attrib-text", "text", attrib->Value(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-int");
			XMLTest("attrib-int", int(1), attrib->IntValue(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-unsigned");
			XMLTest("attrib-unsigned", unsigned(2), attrib->UnsignedValue(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-int64");
			XMLTest("attrib-int64", int64_t(3), attrib->Int64Value(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-uint64");
			XMLTest("attrib-uint64", uint64_t(37), attrib->Unsigned64Value(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-bool");
			XMLTest("attrib-bool", true, attrib->BoolValue(), true);
			attrib = cdoc.FirstChildElement("foo")->FindAttribute("attrib-double");
			XMLTest("attrib-double", 4.0, attrib->DoubleValue(), true);
		}
		// Add API_testcatse :PushDeclaration();PushText();PushComment()
		{
			FILE* fp1 = fopen("resources/out/printer_1.xml", "w");
			XMLPrinter printer(fp1);

			printer.PushDeclaration("version = '1.0' enconding = 'utf-8'");

			printer.OpenElement("foo");
			printer.PushAttribute("attrib-text", "text");

			printer.OpenElement("text");
			printer.PushText("Tinyxml2");
			printer.CloseElement();

			printer.OpenElement("int");
			printer.PushText(int(11));
			printer.CloseElement();

			printer.OpenElement("unsigned");
			printer.PushText(unsigned(12));
			printer.CloseElement();

			printer.OpenElement("int64_t");
			printer.PushText(int64_t(13));
			printer.CloseElement();

			printer.OpenElement("uint64_t");
			printer.PushText(uint64_t(14));
			printer.CloseElement();

			printer.OpenElement("bool");
			printer.PushText(true);
			printer.CloseElement();

			printer.OpenElement("float");
			printer.PushText("1.56");
			printer.CloseElement();

			printer.OpenElement("double");
			printer.PushText("12.12");
			printer.CloseElement();

			printer.OpenElement("comment");
			printer.PushComment("this is Tinyxml2");
			printer.CloseElement();

			printer.CloseElement();
			fclose(fp1);
		}
		{
			XMLDocument doc;
			doc.LoadFile("resources/out/printer_1.xml");
			XMLTest("XMLPrinter Stream mode: load", XML_SUCCESS, doc.ErrorID(), true);

			const XMLDocument& cdoc = doc;

			const  XMLElement* root = cdoc.FirstChildElement("foo");

			const char* text_value;
			text_value = root->FirstChildElement("text")->GetText();
			XMLTest("PushText( const char* text, bool cdata=false ) test", "Tinyxml2", text_value);

			int  int_value;
			int_value = root->FirstChildElement("int")->IntText();
			XMLTest("PushText( int value ) test", 11, int_value);

			unsigned  unsigned_value;
			unsigned_value = root->FirstChildElement("unsigned")->UnsignedText();
			XMLTest("PushText( unsigned value ) test", (unsigned)12, unsigned_value);

			int64_t  int64_t_value;
			int64_t_value = root->FirstChildElement("int64_t")->Int64Text();
			XMLTest("PushText( int64_t value ) test", (int64_t) 13, int64_t_value);

			uint64_t uint64_t_value;
			uint64_t_value = root->FirstChildElement("uint64_t")->Unsigned64Text();
			XMLTest("PushText( uint64_t value ) test", (uint64_t) 14, uint64_t_value);

			float  float_value;
			float_value = root->FirstChildElement("float")->FloatText();
			XMLTest("PushText( float value ) test", 1.56f, float_value);

			double double_value;
			double_value = root->FirstChildElement("double")->DoubleText();
			XMLTest("PushText( double value ) test", 12.12, double_value);

			bool bool_value;
			bool_value = root->FirstChildElement("bool")->BoolText();
			XMLTest("PushText( bool value ) test", true, bool_value);

			const XMLComment* comment = root->FirstChildElement("comment")->FirstChild()->ToComment();
			const char* comment_value = comment->Value();
			XMLTest("PushComment() test", "this is Tinyxml2", comment_value);

			const XMLDeclaration* declaration = cdoc.FirstChild()->ToDeclaration();
			const char* declaration_value = declaration->Value();
			XMLTest("PushDeclaration() test", "version = '1.0' enconding = 'utf-8'", declaration_value);
		}
	}


	// ---------- CDATA ---------------
	{
		const char* str =	"<xmlElement>"
								"<![CDATA["
									"I am > the rules!\n"
									"...since I make symbolic puns"
								"]]>"
							"</xmlElement>";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "CDATA symbolic puns round 1", false, doc.Error() );
		doc.Print();

		XMLTest( "CDATA parse.", "I am > the rules!\n...since I make symbolic puns",
								 doc.FirstChildElement()->FirstChild()->Value(),
								 false );
	}

	// ----------- CDATA -------------
	{
		const char* str =	"<xmlElement>"
								"<![CDATA["
									"<b>I am > the rules!</b>\n"
									"...since I make symbolic puns"
								"]]>"
							"</xmlElement>";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "CDATA symbolic puns round 2", false, doc.Error() );
		doc.Print();

		XMLTest( "CDATA parse. [ tixml1:1480107 ]",
								 "<b>I am > the rules!</b>\n...since I make symbolic puns",
								 doc.FirstChildElement()->FirstChild()->Value(),
								 false );
	}

	// InsertAfterChild causes crash.
	{
		// InsertBeforeChild and InsertAfterChild causes crash.
		XMLDocument doc;
		XMLElement* parent = doc.NewElement( "Parent" );
		doc.InsertFirstChild( parent );

		XMLElement* childText0 = doc.NewElement( "childText0" );
		XMLElement* childText1 = doc.NewElement( "childText1" );

		XMLNode* childNode0 = parent->InsertEndChild( childText0 );
		XMLTest( "InsertEndChild() return", true, childNode0 == childText0 );
		XMLNode* childNode1 = parent->InsertAfterChild( childNode0, childText1 );
		XMLTest( "InsertAfterChild() return", true, childNode1 == childText1 );

		XMLTest( "Test InsertAfterChild on empty node. ", true, ( childNode1 == parent->LastChild() ) );
	}

	{
		// Entities not being written correctly.
		// From Lynn Allen

		const char* passages =
			"<?xml version=\"1.0\" standalone=\"no\" ?>"
			"<passages count=\"006\" formatversion=\"20020620\">"
				"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
				" It also has &lt;, &gt;, and &amp;, as well as a fake copyright &#xA9;.\"> </psg>"
			"</passages>";

		XMLDocument doc;
		doc.Parse( passages );
		XMLTest( "Entity transformation parse round 1", false, doc.Error() );
		XMLElement* psg = doc.RootElement()->FirstChildElement();
		const char* context = psg->Attribute( "context" );
		const char* expected = "Line 5 has \"quotation marks\" and 'apostrophe marks'. It also has <, >, and &, as well as a fake copyright \xC2\xA9.";

		XMLTest( "Entity transformation: read. ", expected, context, true );

		const char* textFilePath = "resources/out/textfile.txt";
		FILE* textfile = fopen( textFilePath, "w" );
		XMLTest( "Entity transformation: open text file for writing", true, textfile != 0, true );
		if ( textfile )
		{
			XMLPrinter streamer( textfile );
			bool acceptResult = psg->Accept( &streamer );
			fclose( textfile );
			XMLTest( "Entity transformation: Accept", true, acceptResult );
		}

		textfile = fopen( textFilePath, "r" );
		XMLTest( "Entity transformation: open text file for reading", true, textfile != 0, true );
		if ( textfile )
		{
			char buf[ 1024 ];
			fgets( buf, 1024, textfile );
			XMLTest( "Entity transformation: write. ",
					 "<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
					 " It also has &lt;, &gt;, and &amp;, as well as a fake copyright \xC2\xA9.\"/>\n",
					 buf, false );
			fclose( textfile );
		}
	}

	{
		// Suppress entities.
		const char* passages =
			"<?xml version=\"1.0\" standalone=\"no\" ?>"
			"<passages count=\"006\" formatversion=\"20020620\">"
				"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;.\">Crazy &ttk;</psg>"
			"</passages>";

		XMLDocument doc( false );
		doc.Parse( passages );
		XMLTest( "Entity transformation parse round 2", false, doc.Error() );

		XMLTest( "No entity parsing.",
				 "Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;.",
				 doc.FirstChildElement()->FirstChildElement()->Attribute( "context" ) );
		XMLTest( "No entity parsing.", "Crazy &ttk;",
				 doc.FirstChildElement()->FirstChildElement()->FirstChild()->Value() );
		doc.Print();
	}

	{
		const char* test = "<?xml version='1.0'?><a.elem xmi.version='2.0'/>";

		XMLDocument doc;
		doc.Parse( test );
		XMLTest( "dot in names", false, doc.Error() );
		XMLTest( "dot in names", "a.elem", doc.FirstChildElement()->Name() );
		XMLTest( "dot in names", "2.0", doc.FirstChildElement()->Attribute( "xmi.version" ) );
	}

	{
		const char* test = "<element><Name>1.1 Start easy ignore fin thickness&#xA;</Name></element>";

		XMLDocument doc;
		doc.Parse( test );
		XMLTest( "fin thickness", false, doc.Error() );

		XMLText* text = doc.FirstChildElement()->FirstChildElement()->FirstChild()->ToText();
		XMLTest( "Entity with one digit.",
				 "1.1 Start easy ignore fin thickness\n", text->Value(),
				 false );
	}

	{
		// DOCTYPE not preserved (950171)
		//
		const char* doctype =
			"<?xml version=\"1.0\" ?>"
			"<!DOCTYPE PLAY SYSTEM 'play.dtd'>"
			"<!ELEMENT title (#PCDATA)>"
			"<!ELEMENT books (title,authors)>"
			"<element />";

		XMLDocument doc;
		doc.Parse( doctype );
		XMLTest( "PLAY SYSTEM parse", false, doc.Error() );
		doc.SaveFile( "resources/out/test7.xml" );
		XMLTest( "PLAY SYSTEM save", false, doc.Error() );
		doc.DeleteChild( doc.RootElement() );
		doc.LoadFile( "resources/out/test7.xml" );
		XMLTest( "PLAY SYSTEM load", false, doc.Error() );
		doc.Print();

		const XMLUnknown* decl = doc.FirstChild()->NextSibling()->ToUnknown();
		XMLTest( "Correct value of unknown.", "DOCTYPE PLAY SYSTEM 'play.dtd'", decl->Value() );

	}

	{
		// Comments do not stream out correctly.
		const char* doctype =
			"<!-- Somewhat<evil> -->";
		XMLDocument doc;
		doc.Parse( doctype );
		XMLTest( "Comment somewhat evil", false, doc.Error() );

		XMLComment* comment = doc.FirstChild()->ToComment();

		XMLTest( "Comment formatting.", " Somewhat<evil> ", comment->Value() );
	}
	{
		// Double attributes
		const char* doctype = "<element attr='red' attr='blue' />";

		XMLDocument doc;
		doc.Parse( doctype );

		XMLTest( "Parsing repeated attributes.", XML_ERROR_PARSING_ATTRIBUTE, doc.ErrorID() );	// is an  error to tinyxml (didn't use to be, but caused issues)
		doc.PrintError();
	}

	{
		// Embedded null in stream.
		const char* doctype = "<element att\0r='red' attr='blue' />";

		XMLDocument doc;
		doc.Parse( doctype );
		XMLTest( "Embedded null throws error.", true, doc.Error() );
	}

	{
		// Empty documents should return TIXML_XML_ERROR_PARSING_EMPTY, bug 1070717
		const char* str = "";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "Empty document error", XML_ERROR_EMPTY_DOCUMENT, doc.ErrorID() );

		// But be sure there is an error string!
		const char* errorStr = doc.ErrorStr();
		XMLTest("Error string should be set",
			"Error=XML_ERROR_EMPTY_DOCUMENT ErrorID=13 (0xd) Line number=0",
			errorStr);
	}

	{
		// Documents with all whitespaces should return TIXML_XML_ERROR_PARSING_EMPTY, bug 1070717
		const char* str = "    ";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( "All whitespaces document error", XML_ERROR_EMPTY_DOCUMENT, doc.ErrorID() );
	}

	{
		// Low entities
		XMLDocument doc;
		doc.Parse( "<test>&#x0e;</test>" );
		XMLTest( "Hex values", false, doc.Error() );
		const char result[] = { 0x0e, 0 };
		XMLTest( "Low entities.", result, doc.FirstChildElement()->GetText() );
		doc.Print();
	}

	{
		// Attribute values with trailing quotes not handled correctly
		XMLDocument doc;
		doc.Parse( "<foo attribute=bar\" />" );
		XMLTest( "Throw error with bad end quotes.", true, doc.Error() );
	}

	{
		// [ 1663758 ] Failure to report error on bad XML
		XMLDocument xml;
		xml.Parse("<x>");
		XMLTest("Missing end tag at end of input", true, xml.Error());
		xml.Parse("<x> ");
		XMLTest("Missing end tag with trailing whitespace", true, xml.Error());
		xml.Parse("<x></y>");
		XMLTest("Mismatched tags", XML_ERROR_MISMATCHED_ELEMENT, xml.ErrorID() );
	}


	{
		// [ 1475201 ] TinyXML parses entities in comments
		XMLDocument xml;
		xml.Parse("<!-- declarations for <head> & <body> -->"
				  "<!-- far &amp; away -->" );
		XMLTest( "Declarations for head and body", false, xml.Error() );

		XMLNode* e0 = xml.FirstChild();
		XMLNode* e1 = e0->NextSibling();
		XMLComment* c0 = e0->ToComment();
		XMLComment* c1 = e1->ToComment();

		XMLTest( "Comments ignore entities.", " declarations for <head> & <body> ", c0->Value(), true );
		XMLTest( "Comments ignore entities.", " far &amp; away ", c1->Value(), true );
	}

	{
		XMLDocument xml;
		xml.Parse( "<Parent>"
						"<child1 att=''/>"
						"<!-- With this comment, child2 will not be parsed! -->"
						"<child2 att=''/>"
					"</Parent>" );
		XMLTest( "Comments iteration", false, xml.Error() );
		xml.Print();

		int count = 0;

		for( XMLNode* ele = xml.FirstChildElement( "Parent" )->FirstChild();
			 ele;
			 ele = ele->NextSibling() )
		{
			++count;
		}

		XMLTest( "Comments iterate correctly.", 3, count );
	}

	{
		// trying to repro [1874301]. If it doesn't go into an infinite loop, all is well.
		unsigned char buf[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?><feed><![CDATA[Test XMLblablablalblbl";
		buf[60] = 239;
		buf[61] = 0;

		XMLDocument doc;
		doc.Parse( (const char*)buf);
		XMLTest( "Broken CDATA", true, doc.Error() );
	}


	{
		// bug 1827248 Error while parsing a little bit malformed file
		// Actually not malformed - should work.
		XMLDocument xml;
		xml.Parse( "<attributelist> </attributelist >" );
		XMLTest( "Handle end tag whitespace", false, xml.Error() );
	}

	{
		// This one must not result in an infinite loop
		XMLDocument xml;
		xml.Parse( "<infinite>loop" );
		XMLTest( "No closing element", true, xml.Error() );
		XMLTest( "Infinite loop test.", true, true );
	}
#endif
	{
		const char* pub = "<?xml version='1.0'?> <element><sub/></element> <!--comment--> <!DOCTYPE>";
		XMLDocument doc;
		doc.Parse( pub );
		XMLTest( "Trailing DOCTYPE", false, doc.Error() );

		XMLDocument clone;
		for( const XMLNode* node=doc.FirstChild(); node; node=node->NextSibling() ) {
			XMLNode* copy = node->ShallowClone( &clone );
			clone.InsertEndChild( copy );
		}

		clone.Print();

		int count=0;
		const XMLNode* a=clone.FirstChild();
		const XMLNode* b=doc.FirstChild();
		for( ; a && b; a=a->NextSibling(), b=b->NextSibling() ) {
			++count;
			XMLTest( "Clone and Equal", true, a->ShallowEqual( b ));
		}
		XMLTest( "Clone and Equal", 4, count );
	}

	{
		// Deep Cloning of root element.
		XMLDocument doc2;
		XMLPrinter printer1;
		{
			// Make sure doc1 is deleted before we test doc2
			const char* xml =
				"<root>"
				"    <child1 foo='bar'/>"
				"    <!-- comment thing -->"
				"    <child2 val='1'>Text</child2>"
				"</root>";
			XMLDocument doc;
			doc.Parse(xml);
			XMLTest( "Parse before deep cloning root element", false, doc.Error() );

			doc.Print(&printer1);
			XMLNode* root = doc.RootElement()->DeepClone(&doc2);
			doc2.InsertFirstChild(root);
		}
		XMLPrinter printer2;
		doc2.Print(&printer2);

		XMLTest("Deep clone of element.", printer1.CStr(), printer2.CStr(), true);
	}

	{
		// Deep Cloning of sub element.
		XMLDocument doc2;
		XMLPrinter printer1;
		{
			// Make sure doc1 is deleted before we test doc2
			const char* xml =
				"<?xml version ='1.0'?>"
				"<root>"
				"    <child1 foo='bar'/>"
				"    <!-- comment thing -->"
				"    <child2 val='1'>Text</child2>"
				"</root>";
			XMLDocument doc;
			doc.Parse(xml);
			XMLTest( "Parse before deep cloning sub element", false, doc.Error() );

			const XMLElement* subElement = doc.FirstChildElement("root")->FirstChildElement("child2");
			bool acceptResult = subElement->Accept(&printer1);
			XMLTest( "Accept before deep cloning", true, acceptResult );

			XMLNode* clonedSubElement = subElement->DeepClone(&doc2);
			doc2.InsertFirstChild(clonedSubElement);
		}
		XMLPrinter printer2;
		doc2.Print(&printer2);

		XMLTest("Deep clone of sub-element.", printer1.CStr(), printer2.CStr(), true);
	}

	{
		// Deep cloning of document.
		XMLDocument doc2;
		XMLPrinter printer1;
		{
			// Make sure doc1 is deleted before we test doc2
			const char* xml =
				"<?xml version ='1.0'?>"
				"<!-- Top level comment. -->"
				"<root>"
				"    <child1 foo='bar'/>"
				"    <!-- comment thing -->"
				"    <child2 val='1'>Text</child2>"
				"</root>";
			XMLDocument doc;
			doc.Parse(xml);
			XMLTest( "Parse before deep cloning document", false, doc.Error() );
			doc.Print(&printer1);

			doc.DeepCopy(&doc2);
		}
		XMLPrinter printer2;
		doc2.Print(&printer2);

		XMLTest("DeepCopy of document.", printer1.CStr(), printer2.CStr(), true);
	}


 	{
		// This shouldn't crash.
		XMLDocument doc;
		if(XML_SUCCESS != doc.LoadFile( "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ))
		{
			doc.PrintError();
		}
		XMLTest( "Error in snprinf handling.", true, doc.Error() );
	}

	{
		// Attribute ordering.
		static const char* xml = "<element attrib1=\"1\" attrib2=\"2\" attrib3=\"3\" />";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse for attribute ordering", false, doc.Error() );
		XMLElement* ele = doc.FirstChildElement();

		const XMLAttribute* a = ele->FirstAttribute();
		XMLTest( "Attribute order", "1", a->Value() );
		a = a->Next();
		XMLTest( "Attribute order", "2", a->Value() );
		a = a->Next();
		XMLTest( "Attribute order", "3", a->Value() );
		XMLTest( "Attribute order", "attrib3", a->Name() );

		ele->DeleteAttribute( "attrib2" );
		a = ele->FirstAttribute();
		XMLTest( "Attribute order", "1", a->Value() );
		a = a->Next();
		XMLTest( "Attribute order", "3", a->Value() );

		ele->DeleteAttribute( "attrib1" );
		ele->DeleteAttribute( "attrib3" );
		XMLTest( "Attribute order (empty)", true, ele->FirstAttribute() == 0 );
	}

	{
		// Make sure an attribute with a space in it succeeds.
		static const char* xml0 = "<element attribute1= \"Test Attribute\"/>";
		static const char* xml1 = "<element attribute1 =\"Test Attribute\"/>";
		static const char* xml2 = "<element attribute1 = \"Test Attribute\"/>";
		XMLDocument doc0;
		doc0.Parse( xml0 );
		XMLTest( "Parse attribute with space 1", false, doc0.Error() );
		XMLDocument doc1;
		doc1.Parse( xml1 );
		XMLTest( "Parse attribute with space 2", false, doc1.Error() );
		XMLDocument doc2;
		doc2.Parse( xml2 );
		XMLTest( "Parse attribute with space 3", false, doc2.Error() );

		XMLElement* ele = 0;
		ele = doc0.FirstChildElement();
		XMLTest( "Attribute with space #1", "Test Attribute", ele->Attribute( "attribute1" ) );
		ele = doc1.FirstChildElement();
		XMLTest( "Attribute with space #2", "Test Attribute", ele->Attribute( "attribute1" ) );
		ele = doc2.FirstChildElement();
		XMLTest( "Attribute with space #3", "Test Attribute", ele->Attribute( "attribute1" ) );
	}

	{
		// Make sure we don't go into an infinite loop.
		static const char* xml = "<doc><element attribute='attribute'/><element attribute='attribute'/></doc>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse two elements with attribute", false, doc.Error() );
		XMLElement* ele0 = doc.FirstChildElement()->FirstChildElement();
		XMLElement* ele1 = ele0->NextSiblingElement();
		bool equal = ele0->ShallowEqual( ele1 );

		XMLTest( "Infinite loop in shallow equal.", true, equal );
	}

	// -------- Handles ------------
	{
		static const char* xml = "<element attrib='bar'><sub>Text</sub></element>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Handle, parse element with attribute and nested element", false, doc.Error() );

		{
			XMLElement* ele = XMLHandle( doc ).FirstChildElement( "element" ).FirstChild().ToElement();
			XMLTest( "Handle, non-const, element is found", true, ele != 0 );
			XMLTest( "Handle, non-const, element name matches", "sub", ele->Value() );
		}

		{
			XMLHandle docH( doc );
			XMLElement* ele = docH.FirstChildElement( "noSuchElement" ).FirstChildElement( "element" ).ToElement();
			XMLTest( "Handle, non-const, element not found", true, ele == 0 );
		}

		{
			const XMLElement* ele = XMLConstHandle( doc ).FirstChildElement( "element" ).FirstChild().ToElement();
			XMLTest( "Handle, const, element is found", true, ele != 0 );
			XMLTest( "Handle, const, element name matches", "sub", ele->Value() );
		}

		{
			XMLConstHandle docH( doc );
			const XMLElement* ele = docH.FirstChildElement( "noSuchElement" ).FirstChildElement( "element" ).ToElement();
			XMLTest( "Handle, const, element not found", true, ele == 0 );
		}
	}
	{
		// Default Declaration & BOM
		XMLDocument doc;
		doc.InsertEndChild( doc.NewDeclaration() );
		doc.SetBOM( true );

		XMLPrinter printer;
		doc.Print( &printer );

		static const char* result  = "\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
		XMLTest( "BOM and default declaration", result, printer.CStr(), false );
		XMLTest( "CStrSize", 42, printer.CStrSize(), false );
	}
	{
		const char* xml = "<ipxml ws='1'><info bla=' /></ipxml>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Ill formed XML", true, doc.Error() );
	}

    {
        //API:IntText(),UnsignedText(),Int64Text(),DoubleText(),BoolText() and FloatText() test
        const char* xml = "<point> <IntText>-24</IntText> <UnsignedText>42</UnsignedText> \
						   <Int64Text>38</Int64Text> <BoolText>true</BoolText> <DoubleText>2.35</DoubleText> </point>";
        XMLDocument doc;
        doc.Parse(xml);

        const XMLElement* pointElement = doc.RootElement();
        int test1 = pointElement->FirstChildElement("IntText")->IntText();
        XMLTest("IntText() test", -24, test1);

        unsigned test2 = pointElement->FirstChildElement("UnsignedText")->UnsignedText();
        XMLTest("UnsignedText() test", static_cast<unsigned>(42), test2);

        int64_t test3 = pointElement->FirstChildElement("Int64Text")->Int64Text();
        XMLTest("Int64Text() test", static_cast<int64_t>(38), test3);

        double test4 = pointElement->FirstChildElement("DoubleText")->DoubleText();
        XMLTest("DoubleText() test", 2.35, test4);

        float test5 = pointElement->FirstChildElement("DoubleText")->FloatText();
        XMLTest("FloatText()) test", 2.35f, test5);

        bool test6 = pointElement->FirstChildElement("BoolText")->BoolText();
        XMLTest("FloatText()) test", true, test6);
    }

    {
        // hex value test
        const char* xml = "<point> <IntText>  0x2020</IntText> <UnsignedText>0X2020</UnsignedText> \
						   <Int64Text> 0x1234</Int64Text></point>";
        XMLDocument doc;
        doc.Parse(xml);

        const XMLElement* pointElement = doc.RootElement();
        int test1 = pointElement->FirstChildElement("IntText")->IntText();
        XMLTest("IntText() hex value test", 0x2020, test1);

        unsigned test2 = pointElement->FirstChildElement("UnsignedText")->UnsignedText();
        XMLTest("UnsignedText() hex value test", static_cast<unsigned>(0x2020), test2);

        int64_t test3 = pointElement->FirstChildElement("Int64Text")->Int64Text();
        XMLTest("Int64Text() hex value test", static_cast<int64_t>(0x1234), test3);
    }

	{
		//API:ShallowEqual() test
		const char* xml = "<playlist id = 'playlist'>"
						    "<property name = 'track_name'>voice</property>"
						  "</playlist>";
		XMLDocument doc;
		doc.Parse( xml );
		const XMLNode* PlaylistNode = doc.RootElement();
		const XMLNode* PropertyNode = PlaylistNode->FirstChildElement();
		bool result;
		result = PlaylistNode->ShallowEqual(PropertyNode);
		XMLTest("ShallowEqual() test",false,result);
		result = PlaylistNode->ShallowEqual(PlaylistNode);
		XMLTest("ShallowEqual() test",true,result);
	}

	{
		//API: previousSiblingElement() and NextSiblingElement() test
		const char* xml = "<playlist id = 'playlist'>"
						    "<property name = 'track_name'>voice</property>"
						    "<entry out = '946' producer = '2_playlist1' in = '0'/>"
							"<blank length = '1'/>"
						  "</playlist>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLElement* ElementPlaylist = doc.FirstChildElement("playlist");
		XMLTest("previousSiblingElement() test",true,ElementPlaylist != 0);
		const XMLElement* pre = ElementPlaylist->PreviousSiblingElement();
		XMLTest("previousSiblingElement() test",true,pre == 0);
		const XMLElement* ElementBlank = ElementPlaylist->FirstChildElement("entry")->NextSiblingElement("blank");
		XMLTest("NextSiblingElement() test",true,ElementBlank != 0);
		const XMLElement* next = ElementBlank->NextSiblingElement();
		XMLTest("NextSiblingElement() test",true,next == 0);
		const XMLElement* ElementEntry = ElementBlank->PreviousSiblingElement("entry");
		XMLTest("PreviousSiblingElement test",true,ElementEntry != 0);
	}

	// QueryXYZText
	{
		const char* xml = "<point> <x>1.2</x> <y>1</y> <z>38</z> <valid>true</valid> </point>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse points", false, doc.Error() );

		const XMLElement* pointElement = doc.RootElement();

		{
			int intValue = 0;
			XMLError queryResult = pointElement->FirstChildElement( "y" )->QueryIntText( &intValue );
			XMLTest( "QueryIntText result", XML_SUCCESS, queryResult, false );
			XMLTest( "QueryIntText", 1, intValue, false );
		}

		{
			unsigned unsignedValue = 0;
			XMLError queryResult = pointElement->FirstChildElement( "y" )->QueryUnsignedText( &unsignedValue );
			XMLTest( "QueryUnsignedText result", XML_SUCCESS, queryResult, false );
			XMLTest( "QueryUnsignedText", (unsigned)1, unsignedValue, false );
		}

		{
			float floatValue = 0;
			XMLError queryResult = pointElement->FirstChildElement( "x" )->QueryFloatText( &floatValue );
			XMLTest( "QueryFloatText result", XML_SUCCESS, queryResult, false );
			XMLTest( "QueryFloatText", 1.2f, floatValue, false );
		}

		{
			double doubleValue = 0;
			XMLError queryResult = pointElement->FirstChildElement( "x" )->QueryDoubleText( &doubleValue );
			XMLTest( "QueryDoubleText result", XML_SUCCESS, queryResult, false );
			XMLTest( "QueryDoubleText", 1.2, doubleValue, false );
		}

		{
			bool boolValue = false;
			XMLError queryResult = pointElement->FirstChildElement( "valid" )->QueryBoolText( &boolValue );
			XMLTest( "QueryBoolText result", XML_SUCCESS, queryResult, false );
			XMLTest( "QueryBoolText", true, boolValue, false );
		}
	}

	{
		const char* xml = "<element><_sub/><:sub/><sub:sub/><sub-sub/></element>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Non-alpha element lead letter parses.", false, doc.Error() );
	}

    {
        const char* xml = "<element _attr1=\"foo\" :attr2=\"bar\"></element>";
        XMLDocument doc;
        doc.Parse( xml );
        XMLTest("Non-alpha attribute lead character parses.", false, doc.Error());
    }

    {
        const char* xml = "<3lement></3lement>";
        XMLDocument doc;
        doc.Parse( xml );
        XMLTest("Element names with lead digit fail to parse.", true, doc.Error());
    }

	{
		const char* xml = "<element/>WOA THIS ISN'T GOING TO PARSE";
		XMLDocument doc;
		doc.Parse( xml, 10 );
		XMLTest( "Set length of incoming data", false, doc.Error() );
	}

    {
        XMLDocument doc;
        XMLTest( "Document is initially empty", true, doc.NoChildren() );
        doc.Clear();
        XMLTest( "Empty is empty after Clear()", true, doc.NoChildren() );
        doc.LoadFile( "resources/dream.xml" );
        XMLTest( "Load dream.xml", false, doc.Error() );
        XMLTest( "Document has something to Clear()", false, doc.NoChildren() );
        doc.Clear();
        XMLTest( "Document Clear()'s", true, doc.NoChildren() );
    }

    {
        XMLDocument doc;
        XMLTest( "No error initially", false, doc.Error() );
        XMLError error = doc.Parse( "This is not XML" );
        XMLTest( "Error after invalid XML", true, doc.Error() );
        XMLTest( "Error after invalid XML", error, doc.ErrorID() );
        doc.Clear();
        XMLTest( "No error after Clear()", false, doc.Error() );
    }

	// ----------- Whitespace ------------
	{
		const char* xml = "<element>"
							"<a> This \nis &apos;  text  &apos; </a>"
							"<b>  This is &apos; text &apos;  \n</b>"
							"<c>This  is  &apos;  \n\n text &apos;</c>"
						  "</element>";
		XMLDocument doc( true, COLLAPSE_WHITESPACE );
		doc.Parse( xml );
		XMLTest( "Parse with whitespace collapsing and &apos", false, doc.Error() );

		const XMLElement* element = doc.FirstChildElement();
		for( const XMLElement* parent = element->FirstChildElement();
			 parent;
			 parent = parent->NextSiblingElement() )
		{
			XMLTest( "Whitespace collapse", "This is ' text '", parent->GetText() );
		}
	}

#if 0
	{
		// Passes if assert doesn't fire.
		XMLDocument xmlDoc;

	    xmlDoc.NewDeclaration();
	    xmlDoc.NewComment("Configuration file");

	    XMLElement *root = xmlDoc.NewElement("settings");
	    root->SetAttribute("version", 2);
	}
#endif

	{
		const char* xml = "<element>    </element>";
		XMLDocument doc( true, COLLAPSE_WHITESPACE );
		doc.Parse( xml );
		XMLTest( "Parse with all whitespaces", false, doc.Error() );
		XMLTest( "Whitespace  all space", true, 0 == doc.FirstChildElement()->FirstChild() );
	}

	// ----------- Preserve Whitespace ------------
	{
		const char* xml = "<element>This  is  &apos;  \n\n text &apos;</element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", "This  is  '  \n\n text '", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element> This \nis &apos;  text  &apos;  </element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", " This \nis '  text  '  ", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  \n This is &apos; text &apos;  \n</element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", "  \n This is ' text '  \n", doc.FirstChildElement()->GetText());
	}

	// Following cases are for text that is all whitespace which are not preserved intentionally
	{
		const char* xml = "<element> </element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", true, 0 == doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>   </element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", true, 0 == doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>\n\n</element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", true, 0 == doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  \n</element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", true, 0 == doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element> \n \n </element>";
		XMLDocument doc(true, PRESERVE_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with whitespace preserved", false, doc.Error());
		XMLTest("Whitespace preserved", true, 0 == doc.FirstChildElement()->GetText());
	}

	// ----------- Pedantic Whitespace ------------
	{
		const char* xml = "<element>This  is  &apos;  \n\n text &apos;</element>";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", "This  is  '  \n\n text '", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element> This \nis &apos;  text  &apos;  </element>";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", " This \nis '  text  '  ", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  \n This is &apos; text &apos;  \n</element>";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", "  \n This is ' text '  \n", doc.FirstChildElement()->GetText());
	}

	// Following cases are for text that is all whitespace which is preserved with pedantic mode
	{
		const char* xml = "<element> </element>";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", " ", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>   </element>";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", "   ", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>\n\n</element>\n";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", "\n\n", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  \n</element> \n ";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", "  \n", doc.FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element> \n  \n </element>  ";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", " \n  \n ", doc.FirstChildElement()->GetText());
	}

	// Following cases are for checking nested elements are still parsed with pedantic whitespace
	{
		const char* xml = "<element>\n\t<a> This is nested text </a>\n</element>  ";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse nested elements with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", " This is nested text ", doc.RootElement()->FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  <b> </b>  </element>\n";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse nested elements with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", " ", doc.RootElement()->FirstChildElement()->GetText());
	}

	{
		const char* xml = "<element>  <c attribute=\"test\"/>  </element>\n ";
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.Parse(xml);
		XMLTest("Parse nested elements with pedantic whitespace", false, doc.Error());
		XMLTest("Pedantic whitespace", true, 0 == doc.RootElement()->FirstChildElement()->GetText());
	}

	// Check sample xml can be parsed with pedantic mode
	{
		XMLDocument doc(true, PEDANTIC_WHITESPACE);
		doc.LoadFile("resources/dream.xml");
		XMLTest("Load dream.xml with pedantic whitespace mode", false, doc.Error());

		XMLTest("Dream", "xml version=\"1.0\"",
			doc.FirstChild()->ToDeclaration()->Value());
		XMLTest("Dream", true, doc.FirstChild()->NextSibling()->ToUnknown() != 0);
		XMLTest("Dream", "DOCTYPE PLAY SYSTEM \"play.dtd\"",
			doc.FirstChild()->NextSibling()->ToUnknown()->Value());
		XMLTest("Dream", "And Robin shall restore amends.",
			doc.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText());
	}

	{
		// An assert should not fire.
		const char* xml = "<element/>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse with self-closed element", false, doc.Error() );
		XMLElement* ele = doc.NewElement( "unused" );		// This will get cleaned up with the 'doc' going out of scope.
		XMLTest( "Tracking unused elements", true, ele != 0, false );
	}


	{
		const char* xml = "<parent><child>abc</child></parent>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse for printing of sub-element", false, doc.Error() );
		XMLElement* ele = doc.FirstChildElement( "parent")->FirstChildElement( "child");

		XMLPrinter printer;
		bool acceptResult = ele->Accept( &printer );
		XMLTest( "Accept of sub-element", true, acceptResult );
		XMLTest( "Printing of sub-element", "<child>abc</child>\n", printer.CStr(), false );
	}


	{
		XMLDocument doc;
		XMLError error = doc.LoadFile( "resources/empty.xml" );
		XMLTest( "Loading an empty file", XML_ERROR_EMPTY_DOCUMENT, error );
		XMLTest( "Loading an empty file and ErrorName as string", "XML_ERROR_EMPTY_DOCUMENT", doc.ErrorName() );
		doc.PrintError();
	}

	{
        // BOM preservation
        static const char* xml_bom_preservation  = "\xef\xbb\xbf<element/>\n";
        {
			XMLDocument doc;
			XMLTest( "BOM preservation (parse)", XML_SUCCESS, doc.Parse( xml_bom_preservation ), false );
            XMLPrinter printer;
            doc.Print( &printer );

            XMLTest( "BOM preservation (compare)", xml_bom_preservation, printer.CStr(), false, true );
			doc.SaveFile( "resources/out/bomtest.xml" );
			XMLTest( "Save bomtest.xml", false, doc.Error() );
        }
		{
			XMLDocument doc;
			doc.LoadFile( "resources/out/bomtest.xml" );
			XMLTest( "Load bomtest.xml", false, doc.Error() );
			XMLTest( "BOM preservation (load)", true, doc.HasBOM(), false );

            XMLPrinter printer;
            doc.Print( &printer );
            XMLTest( "BOM preservation (compare)", xml_bom_preservation, printer.CStr(), false, true );
		}
	}

	{
		// Insertion with Removal
		const char* xml = "<?xml version=\"1.0\" ?>"
			"<root>"
			"<one>"
			"<subtree>"
			"<elem>element 1</elem>text<!-- comment -->"
			"</subtree>"
			"</one>"
			"<two/>"
			"</root>";
		const char* xmlInsideTwo = "<?xml version=\"1.0\" ?>"
			"<root>"
			"<one/>"
			"<two>"
			"<subtree>"
			"<elem>element 1</elem>text<!-- comment -->"
			"</subtree>"
			"</two>"
			"</root>";
		const char* xmlAfterOne = "<?xml version=\"1.0\" ?>"
			"<root>"
			"<one/>"
			"<subtree>"
			"<elem>element 1</elem>text<!-- comment -->"
			"</subtree>"
			"<two/>"
			"</root>";
		const char* xmlAfterTwo = "<?xml version=\"1.0\" ?>"
			"<root>"
			"<one/>"
			"<two/>"
			"<subtree>"
			"<elem>element 1</elem>text<!-- comment -->"
			"</subtree>"
			"</root>";

		XMLDocument doc;
		doc.Parse(xml);
		XMLTest( "Insertion with removal parse round 1", false, doc.Error() );
		XMLElement* subtree = doc.RootElement()->FirstChildElement("one")->FirstChildElement("subtree");
		XMLElement* two = doc.RootElement()->FirstChildElement("two");
		two->InsertFirstChild(subtree);
		XMLPrinter printer1(0, true);
		bool acceptResult = doc.Accept(&printer1);
		XMLTest("Move node from within <one> to <two> - Accept()", true, acceptResult);
		XMLTest("Move node from within <one> to <two>", xmlInsideTwo, printer1.CStr());

		doc.Parse(xml);
		XMLTest( "Insertion with removal parse round 2", false, doc.Error() );
		subtree = doc.RootElement()->FirstChildElement("one")->FirstChildElement("subtree");
		two = doc.RootElement()->FirstChildElement("two");
		doc.RootElement()->InsertAfterChild(two, subtree);
		XMLPrinter printer2(0, true);
		acceptResult = doc.Accept(&printer2);
		XMLTest("Move node from within <one> after <two> - Accept()", true, acceptResult);
		XMLTest("Move node from within <one> after <two>", xmlAfterTwo, printer2.CStr(), false);

		doc.Parse(xml);
		XMLTest( "Insertion with removal parse round 3", false, doc.Error() );
		XMLNode* one = doc.RootElement()->FirstChildElement("one");
		subtree = one->FirstChildElement("subtree");
		doc.RootElement()->InsertAfterChild(one, subtree);
		XMLPrinter printer3(0, true);
		acceptResult = doc.Accept(&printer3);
		XMLTest("Move node from within <one> after <one> - Accept()", true, acceptResult);
		XMLTest("Move node from within <one> after <one>", xmlAfterOne, printer3.CStr(), false);

		doc.Parse(xml);
		XMLTest( "Insertion with removal parse round 4", false, doc.Error() );
		subtree = doc.RootElement()->FirstChildElement("one")->FirstChildElement("subtree");
		two = doc.RootElement()->FirstChildElement("two");
		XMLTest("<two> is the last child at root level", true, two == doc.RootElement()->LastChildElement());
		doc.RootElement()->InsertEndChild(subtree);
		XMLPrinter printer4(0, true);
		acceptResult = doc.Accept(&printer4);
		XMLTest("Move node from within <one> after <two> - Accept()", true, acceptResult);
		XMLTest("Move node from within <one> after <two>", xmlAfterTwo, printer4.CStr(), false);
	}

	{
		const char* xml = "<svg width = \"128\" height = \"128\">"
			"	<text> </text>"
			"</svg>";
		XMLDocument doc;
		doc.Parse(xml);
		XMLTest( "Parse svg with text", false, doc.Error() );
		doc.Print();
	}

	{
		// Test that it doesn't crash.
		const char* xml = "<?xml version=\"1.0\"?><root><sample><field0><1</field0><field1>2</field1></sample></root>";
		XMLDocument doc;
		doc.Parse(xml);
		XMLTest( "Parse root-sample-field0", true, doc.Error() );
		doc.PrintError();
	}

#if 1
		// the question being explored is what kind of print to use:
		// https://github.com/leethomason/tinyxml2/issues/63
	{
		//const char* xml = "<element attrA='123456789.123456789' attrB='1.001e9' attrC='1.0e-10' attrD='1001000000.000000' attrE='0.1234567890123456789'/>";
		const char* xml = "<element/>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse self-closed empty element", false, doc.Error() );
		doc.FirstChildElement()->SetAttribute( "attrA-f64", 123456789.123456789 );
		doc.FirstChildElement()->SetAttribute( "attrB-f64", 1.001e9 );
		doc.FirstChildElement()->SetAttribute( "attrC-f64", 1.0e9 );
		doc.FirstChildElement()->SetAttribute( "attrC-f64", 1.0e20 );
		doc.FirstChildElement()->SetAttribute( "attrD-f64", 1.0e-10 );
		doc.FirstChildElement()->SetAttribute( "attrD-f64", 0.123456789 );

		doc.FirstChildElement()->SetAttribute( "attrA-f32", 123456789.123456789f );
		doc.FirstChildElement()->SetAttribute( "attrB-f32", 1.001e9f );
		doc.FirstChildElement()->SetAttribute( "attrC-f32", 1.0e9f );
		doc.FirstChildElement()->SetAttribute( "attrC-f32", 1.0e20f );
		doc.FirstChildElement()->SetAttribute( "attrD-f32", 1.0e-10f );
		doc.FirstChildElement()->SetAttribute( "attrD-f32", 0.123456789f );

		doc.Print();

		/* The result of this test is platform, compiler, and library version dependent. :("
		XMLPrinter printer;
		doc.Print( &printer );
		XMLTest( "Float and double formatting.",
			"<element attrA-f64=\"123456789.12345679\" attrB-f64=\"1001000000\" attrC-f64=\"1e+20\" attrD-f64=\"0.123456789\" attrA-f32=\"1.2345679e+08\" attrB-f32=\"1.001e+09\" attrC-f32=\"1e+20\" attrD-f32=\"0.12345679\"/>\n",
			printer.CStr(),
			true );
		*/
	}
#endif

    {
        // Issue #184
        // If it doesn't assert, it passes. Caused by objects
        // getting created during parsing which are then
        // inaccessible in the memory pools.
        const char* xmlText = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><test>";
        {
            XMLDocument doc;
            doc.Parse(xmlText);
            XMLTest( "Parse hex no closing tag round 1", true, doc.Error() );
        }
        {
            XMLDocument doc;
            doc.Parse(xmlText);
            XMLTest( "Parse hex no closing tag round 2", true, doc.Error() );
            doc.Clear();
        }
    }

    {
        // If this doesn't assert in TINYXML2_DEBUG, all is well.
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLElement *pRoot = doc.NewElement("Root");
        doc.DeleteNode(pRoot);
    }

    {
        XMLDocument doc;
        XMLElement* root = doc.NewElement( "Root" );
        XMLTest( "Node document before insertion", true, &doc == root->GetDocument() );
        doc.InsertEndChild( root );
        XMLTest( "Node document after insertion", true, &doc == root->GetDocument() );
    }

    {
        // If this doesn't assert in TINYXML2_DEBUG, all is well.
        XMLDocument doc;
        XMLElement* unlinkedRoot = doc.NewElement( "Root" );
        XMLElement* linkedRoot = doc.NewElement( "Root" );
        doc.InsertFirstChild( linkedRoot );
        unlinkedRoot->GetDocument()->DeleteNode( linkedRoot );
        unlinkedRoot->GetDocument()->DeleteNode( unlinkedRoot );
    }

	{
		// Should not assert in TINYXML2_DEBUG
		XMLPrinter printer;
	}

	{
		// Issue 291. Should not crash
		const char* xml = "&#0</a>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( "Parse hex with closing tag", false, doc.Error() );

		XMLPrinter printer;
		doc.Print( &printer );
	}
	{
		// Issue 299. Can print elements that are not linked in.
		// Will crash if issue not fixed.
		XMLDocument doc;
		XMLElement* newElement = doc.NewElement( "printme" );
		XMLPrinter printer;
		bool acceptResult = newElement->Accept( &printer );
		XMLTest( "printme - Accept()", true, acceptResult );
		// Delete the node to avoid possible memory leak report in debug output
		doc.DeleteNode( newElement );
	}
	{
		// Issue 302. Clear errors from LoadFile/SaveFile
		XMLDocument doc;
		XMLTest( "Issue 302. Should be no error initially", "XML_SUCCESS", doc.ErrorName() );
		doc.SaveFile( "./no/such/path/pretty.xml" );
		XMLTest( "Issue 302. Fail to save", "XML_ERROR_FILE_COULD_NOT_BE_OPENED", doc.ErrorName() );
		doc.SaveFile( "./resources/out/compact.xml", true );
		XMLTest( "Issue 302. Subsequent success in saving", "XML_SUCCESS", doc.ErrorName() );
	}

	{
		// If a document fails to load then subsequent
		// successful loads should clear the error
		XMLDocument doc;
		XMLTest( "Should be no error initially", false, doc.Error() );
		doc.LoadFile( "resources/no-such-file.xml" );
		XMLTest( "No such file - should fail", true, doc.Error() );
                
		doc.LoadFile("resources/dream.xml");
		XMLTest("Error should be cleared", false, doc.Error());

		doc.LoadFile( "resources/xmltest-5330.xml" );
        XMLTest( "parse errors occur - should fail", true, doc.Error() );

		doc.LoadFile( "resources/dream.xml" );
		XMLTest( "Error should be cleared", false, doc.Error() );
	}

	{
		// Check that declarations are allowed only at beginning of document
	    const char* xml0 = "<?xml version=\"1.0\" ?>"
	                       "   <!-- xml version=\"1.1\" -->"
	                       "<first />";
	    const char* xml1 = "<?xml version=\"1.0\" ?>"
	                       "<?xml-stylesheet type=\"text/xsl\" href=\"Anything.xsl\"?>"
	                       "<first />";
	    const char* xml2 = "<first />"
	                       "<?xml version=\"1.0\" ?>";
	    const char* xml3 = "<first></first>"
	                       "<?xml version=\"1.0\" ?>";

	    const char* xml4 = "<first><?xml version=\"1.0\" ?></first>";

	    XMLDocument doc;
	    doc.Parse(xml0);
	    XMLTest("Test that the code changes do not affect normal parsing", false, doc.Error() );
	    doc.Parse(xml1);
	    XMLTest("Test that the second declaration is allowed", false, doc.Error() );
	    doc.Parse(xml2);
	    XMLTest("Test that declaration after self-closed child is not allowed", XML_ERROR_PARSING_DECLARATION, doc.ErrorID() );
	    doc.Parse(xml3);
	    XMLTest("Test that declaration after a child is not allowed", XML_ERROR_PARSING_DECLARATION, doc.ErrorID() );
	    doc.Parse(xml4);
	    XMLTest("Test that declaration inside a child is not allowed", XML_ERROR_PARSING_DECLARATION, doc.ErrorID() );
	}

    {
	    // No matter - before or after successfully parsing a text -
	    // calling XMLDocument::Value() used to cause an assert in debug.
	    // Null must be returned.
	    const char* validXml = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
	                           "<first />"
	                           "<second />";
	    XMLDocument* doc = new XMLDocument();
	    XMLTest( "XMLDocument::Value() returns null?", NULL, doc->Value() );
	    doc->Parse( validXml );
	    XMLTest( "Parse to test XMLDocument::Value()", false, doc->Error());
	    XMLTest( "XMLDocument::Value() returns null?", NULL, doc->Value() );
	    delete doc;
    }

	{
		XMLDocument doc;
		for( int i = 0; i < XML_ERROR_COUNT; i++ ) {
			const XMLError error = static_cast<XMLError>(i);
			const char* name = XMLDocument::ErrorIDToName(error);
			XMLTest( "ErrorName() not null after ClearError()", true, name != 0 );
			if( name == 0 ) {
				// passing null pointer into strlen() is undefined behavior, so
				// compiler is allowed to optimise away the null test above if it's
				// as reachable as the strlen() call
				continue;
			}
			XMLTest( "ErrorName() not empty after ClearError()", true, strlen(name) > 0 );
		}
	}

	{
		const char* html("<!DOCTYPE html><html><body><p>test</p><p><br/></p></body></html>");
		XMLDocument doc(false);
		doc.Parse(html);

		XMLPrinter printer(0, true);
		doc.Print(&printer);

		XMLTest(html, html, printer.CStr());
	}

	{
		// Evil memory leaks.
		// If an XMLElement (etc) is allocated via NewElement() (etc.)
		// and NOT added to the XMLDocument, what happens?
		//
		// Previously (buggy):
		//		The memory would be free'd when the XMLDocument is
		//      destructed. But the XMLElement destructor wasn't called, so
		//      memory allocated for the XMLElement text would not be free'd.
		//      In practice this meant strings allocated for the XMLElement
		//      text would be leaked. An edge case, but annoying.
		// Now:
		//      The XMLElement destructor is called. But the unlinked nodes
		//      have to be tracked using a list. This has a minor performance
		//      impact that can become significant if you have a lot of
		//      unlinked nodes. (But why would you do that?)
		// The only way to see this bug was in a Visual C++ runtime debug heap
		// leak tracker. This is compiled in by default on Windows Debug and
		// enabled with _CRTDBG_LEAK_CHECK_DF parameter passed to _CrtSetDbgFlag().
		{
			XMLDocument doc;
			doc.NewElement("LEAK 1");
		}
		{
			XMLDocument doc;
			XMLElement* ele = doc.NewElement("LEAK 2");
			doc.DeleteNode(ele);
		}
	}

	{
		// Bad bad crash. Parsing error results in stack overflow, if uncaught.
		const char* TESTS[] = {
			"./resources/xmltest-5330.xml",
			"./resources/xmltest-4636783552757760.xml",
			"./resources/xmltest-5720541257269248.xml",
			0
		};
		for (int i=0; TESTS[i]; ++i) {
			XMLDocument doc;
			doc.LoadFile(TESTS[i]);
			XMLTest("Stack overflow prevented.", XML_ELEMENT_DEPTH_EXCEEDED, doc.ErrorID());
		}
	}
    {
        const char* TESTS[] = {
            "./resources/xmltest-5662204197076992.xml",     // Security-level performance issue.
            0
        };
        for (int i = 0; TESTS[i]; ++i) {
            XMLDocument doc;
            doc.LoadFile(TESTS[i]);
            // Need only not crash / lock up.
            XMLTest("Fuzz attack prevented.", true, true);
        }
    }
	{
		// Crashing reported via email.
		const char* xml =
			"<playlist id='playlist1'>"
			"<property name='track_name'>voice</property>"
			"<property name='audio_track'>1</property>"
			"<entry out = '604' producer = '4_playlist1' in = '0' />"
			"<blank length = '1' />"
			"<entry out = '1625' producer = '3_playlist' in = '0' />"
			"<blank length = '2' />"
			"<entry out = '946' producer = '2_playlist1' in = '0' />"
			"<blank length = '1' />"
			"<entry out = '128' producer = '1_playlist1' in = '0' />"
			"</playlist>";

		// It's not a good idea to delete elements as you walk the
		// list. I'm not sure this technically should work; but it's
		// an interesting test case.
		XMLDocument doc;
		XMLError err = doc.Parse(xml);
		XMLTest("Crash bug parsing", XML_SUCCESS, err );

		XMLElement* playlist = doc.FirstChildElement("playlist");
		XMLTest("Crash bug parsing", true, playlist != 0);

		{
			const char* elementName = "entry";
			XMLElement* entry = playlist->FirstChildElement(elementName);
			XMLTest("Crash bug parsing", true, entry != 0);
			while (entry) {
				XMLElement* todelete = entry;
				entry = entry->NextSiblingElement(elementName);
				playlist->DeleteChild(todelete);
			}
			entry = playlist->FirstChildElement(elementName);
			XMLTest("Crash bug parsing", true, entry == 0);
		}
		{
			const char* elementName = "blank";
			XMLElement* blank = playlist->FirstChildElement(elementName);
			XMLTest("Crash bug parsing", true, blank != 0);
			while (blank) {
				XMLElement* todelete = blank;
				blank = blank->NextSiblingElement(elementName);
				playlist->DeleteChild(todelete);
			}
			XMLTest("Crash bug parsing", true, blank == 0);
		}

		tinyxml2::XMLPrinter printer;
		const bool acceptResult = playlist->Accept(&printer);
		XMLTest("Crash bug parsing - Accept()", true, acceptResult);
		printf("%s\n", printer.CStr());

		// No test; it only need to not crash.
		// Still, wrap it up with a sanity check
		int nProperty = 0;
		for (const XMLElement* p = playlist->FirstChildElement("property"); p; p = p->NextSiblingElement("property")) {
			nProperty++;
		}
		XMLTest("Crash bug parsing", 2, nProperty);
	}

    // ----------- Line Number Tracking --------------
    {
        struct TestUtil: XMLVisitor
        {
            TestUtil() : str() {}

            void TestParseError(const char *testString, const char *docStr, XMLError expected_error, int expectedLine)
            {
                XMLDocument doc;
                const XMLError parseError = doc.Parse(docStr);

                XMLTest(testString, parseError, doc.ErrorID());
                XMLTest(testString, true, doc.Error());
                XMLTest(testString, expected_error, parseError);
                XMLTest(testString, expectedLine, doc.ErrorLineNum());
            };

            void TestStringLines(const char *testString, const char *docStr, const char *expectedLines)
            {
                XMLDocument doc;
                doc.Parse(docStr);
                XMLTest(testString, false, doc.Error());
                TestDocLines(testString, doc, expectedLines);
            }

            void TestFileLines(const char *testString, const char *file_name, const char *expectedLines)
            {
                XMLDocument doc;
                doc.LoadFile(file_name);
                XMLTest(testString, false, doc.Error());
                TestDocLines(testString, doc, expectedLines);
            }

        private:
            DynArray<char, 10> str;

            void Push(char type, int lineNum)
            {
                str.Push(type);
                str.Push(char('0' + (lineNum / 10)));
                str.Push(char('0' + (lineNum % 10)));
            }

            bool VisitEnter(const XMLDocument& doc)
            {
                Push('D', doc.GetLineNum());
                return true;
            }
            bool VisitEnter(const XMLElement& element, const XMLAttribute* firstAttribute)
            {
                Push('E', element.GetLineNum());
                for (const XMLAttribute *attr = firstAttribute; attr != 0; attr = attr->Next())
                    Push('A', attr->GetLineNum());
                return true;
            }
            bool Visit(const XMLDeclaration& declaration)
            {
                Push('L', declaration.GetLineNum());
                return true;
            }
            bool Visit(const XMLText& text)
            {
                Push('T', text.GetLineNum());
                return true;
            }
            bool Visit(const XMLComment& comment)
            {
                Push('C', comment.GetLineNum());
                return true;
            }
            bool Visit(const XMLUnknown& unknown)
            {
                Push('U', unknown.GetLineNum());
                return true;
            }

            void TestDocLines(const char *testString, XMLDocument &doc, const char *expectedLines)
            {
                str.Clear();
                const bool acceptResult = doc.Accept(this);
                XMLTest(testString, true, acceptResult);
                str.Push(0);
                XMLTest(testString, expectedLines, str.Mem());
            }
        } tester;

		tester.TestParseError("ErrorLine-Parsing", "\n<root>\n foo \n<unclosed/>", XML_ERROR_PARSING, 2);
        tester.TestParseError("ErrorLine-Declaration", "<root>\n<?xml version=\"1.0\"?>", XML_ERROR_PARSING_DECLARATION, 2);
        tester.TestParseError("ErrorLine-Mismatch", "\n<root>\n</mismatch>", XML_ERROR_MISMATCHED_ELEMENT, 2);
        tester.TestParseError("ErrorLine-CData", "\n<root><![CDATA[ \n foo bar \n", XML_ERROR_PARSING_CDATA, 2);
        tester.TestParseError("ErrorLine-Text", "\n<root>\n foo bar \n", XML_ERROR_PARSING_TEXT, 3);
        tester.TestParseError("ErrorLine-Comment", "\n<root>\n<!-- >\n", XML_ERROR_PARSING_COMMENT, 3);
        tester.TestParseError("ErrorLine-Declaration", "\n<root>\n<? >\n", XML_ERROR_PARSING_DECLARATION, 3);
        tester.TestParseError("ErrorLine-Unknown", "\n<root>\n<! \n", XML_ERROR_PARSING_UNKNOWN, 3);
        tester.TestParseError("ErrorLine-Element", "\n<root>\n<unclosed \n", XML_ERROR_PARSING_ELEMENT, 3);
        tester.TestParseError("ErrorLine-Attribute", "\n<root>\n<unclosed \n att\n", XML_ERROR_PARSING_ATTRIBUTE, 4);
        tester.TestParseError("ErrorLine-ElementClose", "\n<root>\n<unclosed \n/unexpected", XML_ERROR_PARSING_ELEMENT, 3);

		tester.TestStringLines(
            "LineNumbers-String",

            "<?xml version=\"1.0\"?>\n"					// 1 Doc, DecL
                "<root a='b' \n"						// 2 Element Attribute
                "c='d'> d <blah/>  \n"					// 3 Attribute Text Element
                "newline in text \n"					// 4 Text
                "and second <zxcv/><![CDATA[\n"			// 5 Element Text
                " cdata test ]]><!-- comment -->\n"		// 6 Comment
                "<! unknown></root>",					// 7 Unknown

            "D01L01E02A02A03T03E03T04E05T05C06U07");

		tester.TestStringLines(
            "LineNumbers-CRLF",

            "\r\n"										// 1 Doc (arguably should be line 2)
            "<?xml version=\"1.0\"?>\n"					// 2 DecL
            "<root>\r\n"								// 3 Element
            "\n"										// 4
            "text contining new line \n"				// 5 Text
            " and also containing crlf \r\n"			// 6
            "<sub><![CDATA[\n"							// 7 Element Text
            "cdata containing new line \n"				// 8
            " and also containing cflr\r\n"				// 9
            "]]></sub><sub2/></root>",					// 10 Element

            "D01L02E03T05E07T07E10");

		tester.TestFileLines(
            "LineNumbers-File",
            "resources/utf8test.xml",
            "D01L01E02E03A03A03T03E04A04A04T04E05A05A05T05E06A06A06T06E07A07A07T07E08A08A08T08E09T09E10T10");
    }

    {
    	const char* xml = "<Hello>Text</Error>";
    	XMLDocument doc;
    	doc.Parse(xml);
    	XMLTest("Test mismatched elements.", true, doc.Error());
    	XMLTest("Test mismatched elements.", XML_ERROR_MISMATCHED_ELEMENT, doc.ErrorID());
    	// For now just make sure calls work & doesn't crash.
    	// May solidify the error output in the future.
    	printf("%s\n", doc.ErrorStr());
    	doc.PrintError();
    }

    // ----------- Performance tracking --------------
	{
#if defined( _MSC_VER )
		__int64 start, end, freq;
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
#endif

		FILE* perfFP = fopen("resources/dream.xml", "r");
		XMLTest("Open dream.xml", true, perfFP != 0);
		fseek(perfFP, 0, SEEK_END);
		long size = ftell(perfFP);
		fseek(perfFP, 0, SEEK_SET);

		char* mem = new char[size + 1];
		memset(mem, 0xfe, size);
		size_t bytesRead = fread(mem, 1, size, perfFP);
		XMLTest("Read dream.xml", true, uint32_t(size) >= uint32_t(bytesRead));
		fclose(perfFP);
		mem[size] = 0;

#if defined( _MSC_VER )
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
#else
		clock_t cstart = clock();
#endif
		bool parseDreamXmlFailed = false;
		static const int COUNT = 10;
		for (int i = 0; i < COUNT; ++i) {
			XMLDocument doc;
			doc.Parse(mem);
			parseDreamXmlFailed = parseDreamXmlFailed || doc.Error();
		}
#if defined( _MSC_VER )
		QueryPerformanceCounter((LARGE_INTEGER*)&end);
#else
		clock_t cend = clock();
#endif
		XMLTest( "Parse dream.xml", false, parseDreamXmlFailed );

		delete[] mem;

		static const char* note =
#ifdef TINYXML2_DEBUG
			"DEBUG";
#else
			"Release";
#endif

#if defined( _MSC_VER )
		const double duration = 1000.0 * (double)(end - start) / ((double)freq * (double)COUNT);
#else
		const double duration = (double)(cend - cstart) / (double)COUNT;
#endif
		printf("\nParsing dream.xml (%s): %.3f milli-seconds\n", note, duration);
	}

#if defined( _MSC_VER ) &&  defined( TINYXML2_DEBUG )
	{
		_CrtMemCheckpoint( &endMemState );

		_CrtMemState diffMemState;
		_CrtMemDifference( &diffMemState, &startMemState, &endMemState );
		_CrtMemDumpStatistics( &diffMemState );

		{
			int leaksBeforeExit = _CrtDumpMemoryLeaks();
			XMLTest( "No leaks before exit?", FALSE, leaksBeforeExit );
		}
	}
#endif

	printf ("\nPass %d, Fail %d\n", gPass, gFail);

	return gFail;
}
