#include "base/xml.h"
#include "base/file.h"
#include <string.h>
#include <stdio.h>
#include <cstdlib>

using namespace base;

int parseInt(const char* c) {
	if((c[0]=='0' && c[1]=='x') || c[0]=='#') { //hex
		c += c[0]=='#'? 1: 2;
		int v=0;
		while(*c) {
			if(*c>='0' && *c<='9') v+= *c-'0';
			else if(*c>='a' && *c<='f') v+= *c-'a'+ 10;
			else if(*c>='A' && *c<='F') v+= *c-'A'+ 10;
			else break;
			v<<=4;
			c++;
		}
		return v>>4;
	} else return atoi(c);
}


XML::Element::Element(): m_type(TAG), m_name(0), m_text(0) {}

const char* XML::Element::operator[](const char* value) const {
	for(std::list<const char*>::const_iterator i=m_attributes.begin(); i!=m_attributes.end(); i++) {
		if(strcmp(value, *i)==0) {
			const char* c=*i;
			while(*c!='"' && *c!='\'') c++;
			return c+1;
		}
	}
	return 0;
}
const char* XML::Element::attribute(const char* name, const char* defaultValue) const {
	const char* v = (*this)[name];
	return v? v: defaultValue;
}
float XML::Element::attribute(const char* name, float defaultValue) const {
	const char* v = (*this)[name];
	return v? atof(v): defaultValue;
}
double XML::Element::attribute(const char* name, double defaultValue) const {
	const char* v = (*this)[name];
	return v? atof(v): defaultValue;
}
int XML::Element::attribute(const char* name, int defaultValue) const {
	const char* v = (*this)[name];
	return v? parseInt(v): defaultValue;
}
bool XML::Element::hasAttribute(const char* value) const {
	return (*this)[value];
}

XML::XML() : m_data(0), m_length(0) {}
XML::~XML() {
	if(m_data) delete [] m_data;
}

bool XML::loadFile(const char* file) {
	char path[64];
	if(File::find(file, path)) file = path;

	FILE* fp = fopen(file, "r");
	if(!fp) return false;
	fseek(fp, 0, SEEK_END);
	m_length = ftell(fp)+1;
	rewind(fp);
	m_data = new char[m_length];
	m_length = fread(m_data, 1, m_length-1, fp) + 1; 
	m_data[m_length-1] = 0;
	return parse()==0;
}
bool XML::loadData(const char* data) {
	m_length = strlen(data);
	m_data = strdup(data);
	return parse()==0;
}

int XML::parse() {
	#define inc(cr) { if(*c==0 || *c=='>') { printf("FAIL [%d]: %s", __LINE__, c); return (int)(c-m_data); } else c++; }

	//build the tree
	char* c = m_data;
	std::list<Element*> stack;
	while(*c) {
		while(*c!='<' && *c!=0) inc(c); //get start of tag
		if(*c==0) break; //EOF
		c++;
		//Terermine tag type
		if(*c=='?') {
		       	while(*c!='>') inc(c); //header
			c++;
		} else if(*c=='!') { //comment or DOCTYPE
			if(strncmp(c, "!DOCTYPE", 8)==0) {
		       		while(*c!='>') inc(c); //Skip
				c++;
			} else if(strncmp(c, "!--", 3)==0) {
				c+=3; // <!-- text -->
				Element t;
				t.m_type = COMMENT;
				t.m_name = c;
				while(strncmp(c, "-->",3)) inc(c);
				*c = 0; c+=3; //terminate the string
				if(!stack.empty()) stack.back()->m_children.push_back(t);
				if(!stack.empty()) stack.back()->m_text=0;
			} else return false;
		} else if(*c=='/') {	//Closing tag
			char *tmp = ++c;
		       	while(*c!='>') inc(c); //get end of tag name
			*c=0;
			if(strcmp(stack.back()->name(), tmp)==0) {
				//Terminate text
				if(stack.back()->m_text) {
					for(char* lc=tmp-3; lc>=stack.back()->m_text && (*lc==' ' || *lc=='\t' || *lc==10 || *lc==13); lc--) *lc=0;
				}
				stack.pop_back();
				if(stack.empty()) return 0;
			} else {
				return (int)(c-m_data); //Error: Invalid file
			}
			c++;
		} else { //tag
			Element t;
			char flag=0;
			t.m_name = c;
			t.m_type = TAG;
			while(*c!=' ' && *c!='\t' && *c!='\n' && *c!='>' && *c!='/') inc(c);
			if(*c=='>' || *c=='/') flag=*c;
			*c = 0; c++; //terminate name string
			//find values
			while(!flag && *c!='>') {
				if(*c!=' ' && *c!='\t' && *c!='\n' && *c!='/') {
					//we have an attribute
					t.m_attributes.push_back(c);
					while(*c!='=' && *c!=' ' && *c!='\t' && *c!='\n') inc(c);
					*c = 0; c++;
					//make sure the value is quoted
					while(*c!='"' && *c!='\'') inc(c);
					char quote = *c; c++;
					while(*c!=quote) inc(c);
					*c = 0; //terminate string
				}
				c++;
			}
			//is this a container?
			//Add tag to the tree
			if(stack.empty()) m_root = t;
			else stack.back()->m_children.push_back(t);
			//Do we need to push onto the stack?
			if(*(c-1)!='/' && flag!='/') {
				if(stack.empty()) stack.push_back(&m_root);
				else stack.push_back(&stack.back()->m_children.back());
				stack.back()->m_text = c; //Internal text?
			}
			c++;
		}
	}
	return (int)(c-m_data);
}


