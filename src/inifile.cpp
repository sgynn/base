#include "base/inifile.h"
#include <cstdio>
#include <cstdlib>

using namespace base;

INIFile::INIFile() { }
INIFile::~INIFile() { }

/** Section pointer */
INIFile::Section* INIFile::section(const char* name) {
	if(m_sections.contains(name)) return m_sections[name];
	else return m_sections.insert(name, new Section(name));
}

/** Operators */
const INIFile::Section INIFile::nullSection("");
const INIFile::Section& INIFile::operator[](const char* name) const {
	if(m_sections.contains(name)) return *m_sections[name];
	else return nullSection;
}
INIFile::Section& INIFile::operator[](const char* name) { return *section(name); }


INIFile INIFile::load(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(!fp) return INIFile();
	fseek(fp, 0, SEEK_END);
	size_t length = ftell(fp);
	rewind(fp);
	char* data = new char[length+1];
	length = fread(data, 1, length, fp);
	data[length] = 0;
	INIFile ini = parse(data);
	delete [] data;
	return ini;
}
bool INIFile::save(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;
	for(HashMap<Section*>::iterator i=m_sections.begin(); i!=m_sections.end(); i++) {
		fprintf(fp, "[%s]\n", i.key());
		Section* s = *i;
		for(HashMap<Value>::iterator j = s->m_values.begin(); j!=s->m_values.end(); j++) {
			fprintf(fp, "%s = %s\n", j.key(), (const char*) *j);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

INIFile INIFile::parse(const char* data) {
	#define whitespace(c) while(*c && (*c==' ' || *c=='\t')) ++c;

	char buffer[128];
	const char* c = data;
	const char* e;
	INIFile ini;
	Section* s = 0;
	while(*c) {
		if(*c=='#' || *c==';') while(*c && *c!='\n') ++c; // Comment
		//Section header
		else if(*c == '[') {
			++c;
			whitespace(c);
			e=c; while(*e && *e!='\n' && *e!=']') ++e; // find end of name
			if(*e==']') {
				while(*e==']' || *e==' ' || *e=='\t') --e; // trim
				if(e>c) { // Make sure name exists
					strncpy(buffer, c, e-c+1);
					buffer[e-c+1] = 0;
					s = ini.section(buffer); // Create section
				}
			}
			while(*c && *c!='\n') ++c;

		} else if(s) { //Attributes
			whitespace(c);
			e=c; while(*e && *e!='\n' && *e!='=' && *e!=';') ++e; // find end of name
			if(*e=='=') {
				while(*e==']' || *e==' ' || *e=='\t') --e; // trim
				if(e>c) { // Make sure name exists
					strncpy(buffer, c, e-c);
					buffer[e-c] = 0;

					// Get value
					c=e+1; whitespace(c); ++c; whitespace(c);
					e=c; while(*e && *e!='\n' && *e!=';') ++e; // find end of name
					while(*e==' ' || *e=='\t') --e; // trim
					if(e>c) { // Make sure value exists
						Value value;
						value.m_source = (char*)malloc(e-c+1);
						strncpy(value.m_source, c, e-c+1);
						s->set(buffer, value);
					}
				} else printf("INI Warning: found '=' with no name\n");
			}
			while(*c && *c!='\n') ++c;
		}
	}
	return ini;
}

//// //// Section functions //// ////
const INIFile::Value INIFile::Section::nullValue;

INIFile::Section::Section(const char* name) {
	strcpy(m_name, name);
}
INIFile::Section::~Section() {
}

const INIFile::Value& INIFile::Section::operator[](const char* c) const { return get(c); }
INIFile::Value& INIFile::Section::operator[](const char* c) { return m_values[c]; }
void INIFile::Section::set(const char* c, const Value& v) { m_values[c] = v; }
const INIFile::Value& INIFile::Section::get(const char* c) const {
	if(m_values.contains(c)) return m_values[c];
	else return nullValue;
}

//// //// Value Functions //// ////
INIFile::Value::operator const char*() {
	switch(m_type) {
	case SOURCE: return m_source;
	case STRING: return m_c;
	default: setString(); return m_source;
	}
}
INIFile::Value::operator float() {
	switch(m_type) {
	case SOURCE: return m_source? atof(m_source): 0;
	case STRING: return atof(m_c);
	case FLOAT:  return m_f;
	case BOOL:   return m_b? 1.0f: 0;
	case INTEGER:return m_i;
	default: return 0;
	}
}
INIFile::Value::operator bool() {
	switch(m_type) {
	case SOURCE: return strcmp(m_source, "1")==0 || strcmp(m_source, "true")==0 || strcmp(m_source, "yes")==0;
	case STRING: return strcmp(m_source, "1")==0 || strcmp(m_source, "true")==0 || strcmp(m_source, "yes")==0;
	case FLOAT:  return m_f!=0;
	case BOOL:   return m_b;
	case INTEGER:return m_i;
	default: return 0;
	}
}
INIFile::Value::operator int() {
	switch(m_type) {
	case SOURCE: return m_source? atoi(m_source): 0;
	case STRING: return atoi(m_c);
	case FLOAT:  return m_f;
	case BOOL:   return m_b? 1: 0;
	case INTEGER:return m_i;
	default: return 0;
	}
}
void INIFile::Value::setType(int t) {
	m_type = t;
	if(m_source) free(m_source);
	m_source = 0;
}
void INIFile::Value::setString() {
	if(!m_source) m_source = (char*)malloc(12);
	switch(m_type) {
	case FLOAT:   sprintf(m_source, "%f", m_f); break;
	case BOOL:    strcpy(m_source, m_b? "true": "false"); break;
	case INTEGER: sprintf(m_source, "%d", m_i); break;
	default: break;
	}
}


