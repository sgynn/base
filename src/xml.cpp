#include "base/xml.h"
#include "base/file.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <assert.h>

using namespace base;

//// Reference counted string functions ////
inline RefString::RefString(const char* str): s(0), ref(0) { 
	if(str) { s=strdup(str); ref=new int(1); }
}
inline RefString::RefString(const RefString& r): s(r.s), ref(r.ref) {
	if(ref) ++*ref;
}
inline const RefString& RefString::operator=(const RefString& r) { 
	drop();
	s = r.s;
	ref = r.ref; 
	if(ref) ++*ref;
	return *this;
}
inline const RefString& RefString::operator=(const char* r) {
	drop();
	if(r) { s=strdup(r); ref=new int(1); }
	return *this;
}
inline void RefString::drop() {
	if(s && *ref==0) printf("Error: Invalid reference %p\n", s);
	if(s && --*ref==0) { free(s); delete ref; s=0; ref=0; } 
}
inline RefString::~RefString() { drop(); }

inline RefString RefString::substr(size_t start, size_t len) {
	size_t slen = s? strlen(s): 0;
	if(start==0 && len>=slen) return RefString(*this);
	else if(start>=slen || len==0) return RefString();
	else {
		if(start+len > slen) len = slen-start;
		RefString sub;
		char tmp = s[start+len];
		s[start+len] = 0;
		sub = s;
		s[start+len] = tmp;
		return sub;
	}
}



/** Parse an integer or hex value */
inline int parseInt(const char* c) {
	char* end;
	if(c[0] == '#') return strtol(c+1, &end, 16);
	return strtol(c, &end, 0);
}
inline unsigned parseUint(const char* c) {
	char* end;
	if(c[0] == '#') return strtoul(c+1, &end, 16);
	return strtoul(c, &end, 0);
}


XMLElement::XMLElement(int type): m_type(type) { }
XMLElement::XMLElement(const char* tag): m_type(XML::TAG), m_name(tag) { }
const char* XMLElement::attribute(const char* name, const char* defaultValue) const {
	if(m_attributes.contains(name)) return m_attributes[name];
	else return defaultValue;
}
float XMLElement::attribute(const char* name, float defaultValue) const {
	const char* v = attribute(name, (const char*)0);
	return v? atof(v): defaultValue;
}
double XMLElement::attribute(const char* name, double defaultValue) const {
	const char* v = attribute(name, (const char*)0);
	return v? atof(v): defaultValue;
}
int XMLElement::attribute(const char* name, int defaultValue) const {
	const char* v = attribute(name, (const char*)0);
	return v? parseInt(v): defaultValue;
}
unsigned XMLElement::attribute(const char* name, unsigned defaultValue) const {
	const char* v = attribute(name, (const char*)0);
	return v? parseUint(v): defaultValue;
}
bool XMLElement::hasAttribute(const char* name) const {
	return m_attributes.contains(name);
}

//// Setting data ////

void XMLElement::setAttribute(const char* name, const char* value) {
	m_attributes[name] = value;
}
void XMLElement::setAttribute(const char* name, double v) {
	char s[16]; sprintf(s, "%g", v);
	setAttribute(name, s);
}
void XMLElement::setAttribute(const char* name, float v) {
	char s[16]; sprintf(s, "%g", v);
	setAttribute(name, s);
}
void XMLElement::setAttribute(const char* name, int v, bool hex) {
	char s[16]; sprintf(s, hex? "#%x": "%d", v);
	setAttribute(name, s);
}
void XMLElement::setText(const char* s) {
	m_text = s;
}

XMLElement& XMLElement::add(const XMLElement& e) {
	m_children.push_back(e);
	return m_children.back();
}

XMLElement& XMLElement::add(const char* tag) {
	return add( XMLElement(tag) );
}

bool XMLElement::operator==(const char* s) const {
	return m_name && strcmp(m_name, s)==0;
}
bool XMLElement::operator!=(const char* s) const {
	return !m_name || strcmp(m_name, s)!=0;
}

XMLElement& XMLElement::find(const char* tag, int index) {
	static XMLElement none;
	for(unsigned i=0; i<size(); ++i) {
		if(m_children[i]==tag && --index<0) return m_children[i];
	}
	return none;
}
const XMLElement& XMLElement::find(const char* tag, int index) const {
	static XMLElement none;
	for(unsigned i=0; i<size(); ++i) {
		if(m_children[i]==tag && --index<0) return m_children[i];
	}
	return none;
}


XML::XML() {}
XML::XML(const char* root): m_root(root) {}

XML XML::load(const char* file) {
	FILE* fp = fopen(file, "r");
	if(!fp) return XML();
	fseek(fp, 0, SEEK_END);
	unsigned len = ftell(fp);
	rewind(fp);
	char* string = new char[len+1];
	fread(string, 1, len, fp); 
	string[len] = 0;

	XML xml;
	xml.parseInternal(string);
	delete [] string;
	fclose(fp);
	return xml;
}
XML XML::parse(const char* string) {
	XML xml;
	char* s = strdup(string);
	xml.parseInternal(s);
	free(s);
	return xml;
}

int XML::save(const char* filename) const {
	FILE* fp = fopen(filename, "w");
	if(!fp) return 0;
	const char* s = toString();
	size_t len = strlen(s);
	size_t w = fwrite(s, 1, len, fp);
	fclose(fp);
	delete [] s;
	return w==len;
}


const char* XML::toString() const {
	#define expand(n) { char* tmp=s; s = new char[len+n]; memcpy(s,tmp,len); len+=n; delete[]tmp; } 
	const char* head = "<?xml version=\"1.0\"?>\n";
	size_t len = 1024;
	size_t p = 0;
	char* s = new char[len];
	sprintf(s, head);
	p = strlen(head);
	std::vector<const Element*> stack;
	std::vector<unsigned> index;
	const Element* e = &m_root;
	unsigned child = 0;
	int n;
	// Construct string
	while(e) {
		if(len < p + 512) expand(1024);
		int tab = stack.size();
		for(int i=0; i<tab; ++i) s[p++] = '\t';
		
		switch(e->type()) {
		case TAG:
			p += sprintf(s+p, "<%s", e->name());
			// Attributes
			for(HashMap<RefString>::const_iterator i=e->m_attributes.begin(); i!=e->m_attributes.end(); ++i) {
				p += sprintf(s+p, " %s=\"%s\"", i.key(), (const char*)*i);
			}
			// Children?
			if(e->size()) { sprintf(s+p, ">\n"); p+=2; }
			else { sprintf(s+p, "/>\n"); p+=3; }
			
		case HEADER:
			break;
		case COMMENT:
			n = strlen(e->text()) + 8;
			if(len < p+n) expand(p+n-len+256);
			sprintf(s+p, "<!--%s-->\n", e->text());
			p += n;
			break;
		}

		// Next element
		if(e->size()) {
			stack.push_back(e);
			index.push_back(child);
			e = &e->child(0);
			child = 0;
		} else {
			++child;
			while(!stack.empty() && child==stack.back()->size()) {
				// Pop stack
				e = stack.back();
				child = index.back() + 1;
				stack.pop_back();
				index.pop_back();

				// Close tag
				int tab = stack.size();
				for(int i=0; i<tab; ++i) s[p++] = '\t';
				p += sprintf(s+p, "</%s>\n", e->name());
			} 
			// Next child element
			if(stack.empty()) e = 0;
			else e = &stack.back()->child( child );
		}
	}
	// Terminate string
	s[p] = 0;
	return s;
}



int XML::parseInternal(char* string) {
	static const char* invalid = "!\"#$%&'()*+,/;<=>?@[\\]^`{|}~";
	#define inc(c) if(*(++c)=='\n') ++line;
	#define isSpace(c) (c==0x20 || c==0x9 || c==0xd || c==0xa ) // SPACE, TAB, CR, LF
	#define whitespace(c)  while( isSpace(*c) ) inc(c); 
	#define fail(error) { printf("XML Parse Error: %s [%.32s]\n", error, c); return (int)(c-string); }

	//build the tree
	int line=0;
	char* c = string;
	std::vector<Element*> stack;
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
				char* comment = c;
				while(*c && strncmp(c, "-->",3)) inc(c);
				if(*c==0) fail("Extected '-->'");
				*c = 0; c+=3; //terminate the string
				// Create comment element
				if(!stack.empty()) {
					stack.back()->add( Element(COMMENT) ).m_text = comment;
				} else printf("Warning: Comment outside root element\n");
			} else return false;
		} else if(*c=='/') {	//Closing tag
			char* name = c+1;
			while(*c && *c!='>') inc(c); //get end of tag name
			if(*c==0) fail("Extected '>'");
			*c=0; ++c;
			// Validate closing tag
			if(strcmp(stack.back()->name(), name)==0) {
				stack.pop_back();
				if(stack.empty()) return 0;
			} else {
				printf("XML: Invalid closing tag '%s'. Expected %s\n", name, stack.back()->name());
				return (int)(c-string); //Error: Invalid file
			}
		} else { //tag
			char* name = c;
			while(*c>32 && strchr(invalid, *c)==0) ++c; // read tagname
			char end = *c; *c = 0;
			// Create element in place
			Element* t;
			if(stack.empty()) t = &m_root;
			else t = &stack.back()->add( Element(TAG) );
			t->m_name = name;
			*c = end; // replace name terminating character
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
				char* value = c;
				//Find end quote
				while(*c && *c!=quote) inc(c);
				// attribute is valid
				if(*c==quote) {
					*ne=0; 		// End of name
					*c=0; ++c;	// End of value
					t->setAttribute(name, value);
				} else fail("No end quote");
				whitespace(c);
			}
			// Validate end
			if(*c=='/' && c[1]!='>') fail("Expected >");
			if(*c==0) fail("Expected '>'");

			// Does this element contain child nodes
			if(*c=='>') {
				if(stack.empty()) stack.push_back(&m_root);
				else stack.push_back(&stack.back()->m_children.back());
				// Internal text - Note: This will be broken by child nodes
				++c; whitespace(c);
				char* text = c;
				while(*c!=0 && *c!='<') inc(c);
				// Erase trailing whitespace
				if(*c=='<') {
					for(char* t=c-1; *t==' '||*t=='\t'||*t=='\n' || *t=='\r'; --t) *t=0;
					if(text[0]!='<' && text[0]!=0) {
						*c = 0;
						stack.back()->m_text = text;
						*c = '<';
					}
				}
			} else c+=2; // "/>"
		}
	}
	return (int)(c-string);
}


