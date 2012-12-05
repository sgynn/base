#include "base/xml.h"
#include "base/file.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace base;

/** Parse an integer or hex value */
int parseInt(const char* c) {
	if((c[0]=='0' && c[1]=='x') || c[0]=='#') { //hex
		c += c[0]=='#'? 1: 2;
		int v;
		for(v=0; *c; ++c) {
			if(*c>='0' && *c<='9') v = (v<<4) + *c-'0';
			else if(*c>='a' && *c<='f') v = (v<<4) + *c-'a'+ 10;
			else if(*c>='A' && *c<='F') v = (v<<4) + *c-'A'+ 10;
			else break;
		}
		return v;
	} else return atoi(c);
}


XMLElement::XMLElement(): m_type(XML::TAG), m_name(0), m_text(0) {}

const char* XMLElement::operator[](const char* value) const {
	for(std::list<const char*>::const_iterator i=m_attributes.begin(); i!=m_attributes.end(); i++) {
		if(strcmp(value, *i)==0) {
			const char* c=*i;
			while(*c!='"' && *c!='\'') c++;
			return c+1;
		}
	}
	return 0;
}
const char* XMLElement::attribute(const char* name, const char* defaultValue) const {
	const char* v = (*this)[name];
	return v? v: defaultValue;
}
float XMLElement::attribute(const char* name, float defaultValue) const {
	const char* v = (*this)[name];
	return v? atof(v): defaultValue;
}
double XMLElement::attribute(const char* name, double defaultValue) const {
	const char* v = (*this)[name];
	return v? atof(v): defaultValue;
}
int XMLElement::attribute(const char* name, int defaultValue) const {
	const char* v = (*this)[name];
	return v? parseInt(v): defaultValue;
}
bool XMLElement::hasAttribute(const char* value) const {
	return (*this)[value];
}

bool XMLElement::operator==(const char* s) const {
	return m_name && strcmp(m_name, s)==0;
}




XML::XML() : m_data(0), m_length(0), m_ref(0) {}
XML::XML(const XML& x): m_root(x.m_root), m_data(x.m_data), m_length(x.m_length), m_ref(x.m_ref) { ++ *m_ref; }
const XML& XML::operator=(const XML& x) {
	m_root = x.m_root;
	m_data=x.m_data; m_length=x.m_length;
	m_ref=x.m_ref; ++*m_ref;
	return *this;
}
XML::~XML() {
	if(m_ref) --*m_ref;
	if(m_data && *m_ref==0) {
		free(m_data);
		delete m_ref;
	}
}

XML XML::load(const char* file) {
	FILE* fp = fopen(file, "r");
	if(!fp) return XML();
	XML xml;
	fseek(fp, 0, SEEK_END);
	xml.m_length = ftell(fp);
	rewind(fp);
	xml.m_data = (char*)malloc(xml.m_length+1);
	xml.m_length = fread(xml.m_data, 1, xml.m_length, fp) + 1; 
	xml.m_data[xml.m_length-1] = 0;
	xml.m_ref = new int(1);
	xml.parse();
	return xml;
}
XML XML::parse(const char* string) {
	XML xml;
	xml.m_length = strlen(string);
	xml.m_data = strdup(string);
	xml.m_ref = new int(1);
	xml.parse();
	return xml;
}

int XML::parse() {
	static const char* invalid = "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~";
	#define inc(c) if(*(++c)=='\n') ++line;
	#define whitespace(c)  while(*c==0x20 || *c==0x9 || *c==0xd || *c==0xa) inc(c); // SPACE, TAB, CR, LF
	#define fail(error) { printf("XML Parse Error: %s [%.32s]\n", error, c); return (int)(c-m_data); }

	//build the tree
	int line=0;
	char* c = m_data;
	std::list<Element*> stack;
	while(*c) {
		while(*c!='<' && *c!=0) inc(c); //get start of tag
		if(*c==0) break; //EOF
		*c=0; ++c;
		//Terermine tag type
		if(*c=='?') {
			while(*c!='>') inc(c); //header
			if(*c==0) fail("Extected '>'");
			++c;
		} else if(*c=='!') { //comment or DOCTYPE
			if(strncmp(c, "!DOCTYPE", 8)==0) {
				while(*c!='>') inc(c); //Skip
				if(*c==0) fail("Extected '>'");
				c++;
			} else if(strncmp(c, "!--", 3)==0) {
				c+=3; // <!-- text -->
				Element t;
				t.m_type = COMMENT;
				t.m_name = 0;
				t.m_text = c;
				while(*c && strncmp(c, "-->",3)) inc(c);
				if(*c==0) fail("Extected '-->'");
				*c = 0; c+=3; //terminate the string
				if(!stack.empty()) stack.back()->m_children.push_back(t);
			} else return false;
		} else if(*c=='/') {	//Closing tag
			char *tmp = ++c;
			while(*c!='>') inc(c); //get end of tag name
			if(*c==0) fail("Extected '>'");
			*c=0; ++c;
			// Validate closing tag
			if(strcmp(stack.back()->name(), tmp)==0) {
				//Terminate text
				if(stack.back()->m_text) {
					for(char* lc=tmp-3; lc>=stack.back()->m_text && (*lc==' ' || *lc=='\t' || *lc==10 || *lc==13); lc--) *lc=0;
				}
				stack.pop_back();
				if(stack.empty()) return 0;
			} else {
				printf("XML: Invalid closing tag '%s'. Expected %s\n", tmp, stack.back()->name());
				return (int)(c-m_data); //Error: Invalid file
			}
		} else { //tag
			Element t;
			t.m_name = c;
			t.m_type = TAG;
			while(*c>32 && strchr(invalid, *c)==0) ++c; // read tagname
			char* endName = c;
			whitespace(c);
			// Read attributes
			while(*c && *c!='>' && *c!='/') {
				// Attribute name
				const char* name = c;
				while(*c>32 && strchr(invalid,*c)==0) ++c;
				char* ne = c;
				whitespace(c);
				// =
				if(*c=='=') ++c;
				else fail("Expected '='");
				whitespace(c);
				// Value
				if(*c!='\'' && *c!='"') fail("Expected \"");
				char quote = *c; ++c;
				//Find end quote
				while(*c && *c!=quote) inc(c);
				// attribute is valid
				if(*c==quote) {
					*ne=0; // End of name
					t.m_attributes.push_back(name);
					*c=0; ++c;
				} else fail("No end quote");
				whitespace(c);
			}
			// Validate end
			if(*c=='/' && c[1]!='>') fail("Expected >");
			if(*c==0) fail("Expected '>'");

			// Add tag to the tree
			if(stack.empty()) m_root = t;
			else stack.back()->m_children.push_back(t);
			// Does this element contain child nodes
			if(*c=='>') {
				if(stack.empty()) stack.push_back(&m_root);
				else stack.push_back(&stack.back()->m_children.back());
				// Internal text - Note: This will be broken by child nodes
				++c; whitespace(c);
				const char* text = c;
				while(*c!=0 && *c!='<') inc(c);
				if(text[0]!='<') stack.back()->m_text = text;
			} else c+=2; // "/>"
			// null terminate tag name string
			*endName=0;  
		}
	}
	return (int)(c-m_data);
}


