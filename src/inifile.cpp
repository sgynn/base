#include "base/inifile.h"
#include <cstdio>
#include <cstdlib>
#include <utility>

using namespace base;

INIFile::INIFile() { }
INIFile::~INIFile() { }

/** Section pointer */
INIFile::Section* INIFile::section(const char* name) {
	if(m_sections.contains(name)) return m_sections[name];
	else return m_sections.insert(name, new Section(name));
}
INIFile::INIFile(INIFile&& o) {
	m_sections = std::move(o.m_sections);
}
INIFile& INIFile::operator=(INIFile&& o) {
	m_sections = std::move(o.m_sections);
	return *this;
}

/** Operators */
const INIFile::Section INIFile::nullSection("");
const INIFile::Section& INIFile::operator[](const char* name) const {
	if(m_sections.contains(name)) return *m_sections[name];
	else return nullSection;
}
INIFile::Section& INIFile::operator[](const char* name) { return *section(name); }
bool INIFile::containsSection(const char* s) const { return m_sections.contains(s); }

INIFile INIFile::load(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(!fp) return INIFile();
	fseek(fp, 0, SEEK_END);
	size_t length = ftell(fp);
	rewind(fp);
	char* data = new char[length+1];
	length = fread(data, 1, length, fp);
	data[length] = 0;
	fclose(fp);
	INIFile ini = parse(data);
	delete [] data;
	return ini;
}
bool INIFile::save(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;
	for(auto& s: m_sections) {
		fprintf(fp, "[%s]\n", s.key);
		for(const KeyValue& v: *s.value) {
			if(v.key) {
				switch(v.value.m_type) {
				case Value::FLOAT:   fprintf(fp, "%s = %g\n", v.key, v.value.m_f); break;
				case Value::BOOL:    fprintf(fp, "%s = %s\n", v.key, v.value.m_b? "true": "false"); break;
				case Value::INTEGER: fprintf(fp, "%s = %d\n", v.key, v.value.m_i); break;
				default:             fprintf(fp, "%s = %s\n", v.key, (const char*)v.value); break;
				}
			}
			else fprintf(fp, "%s\n", (const char*)v.value);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

INIFile INIFile::parse(const char* data) {
	#define whitespace(c) (*c==' ' || *c=='\t')
	#define newline(c) (*c=='\n' || *c=='\r')
	char buffer[128];
	const char* c = data;
	const char* e;
	INIFile ini;
	Section* s = 0;
	while(*c) {
		if(*c=='#' || *c==';') while(*c && !newline(c)) ++c; // Comment
		//Section header
		else if(*c == '[') {
			++c;
			while(*c && whitespace(c)) ++c;
			e=c;
			while(*e && !newline(e) && *e!=']') ++e; // find end of name
			if(*e==']') {
				while(*e==']' || whitespace(e)) --e; // trim
				if(e>c) { // Make sure name exists
					strncpy(buffer, c, e-c+1);
					buffer[e-c+1] = 0;
					s = ini.section(buffer); // Create section
				} else printf("Invalid ini section name\n");
			} else printf("Expected ']' for ini section\n");
			while(*c && !newline(c)) ++c;

		}
		// Attribute values
		else if(s) {
			while(*c && whitespace(c)) ++c;
			e=c;
			while(*e && !newline(e) && *e!='=' && *e!=';' && *e!='#') ++e; // find end of name
			if(*e=='=') {
				const char* v=e+1;
				while(e>=c && (*e=='=' || whitespace(e))) --e; // trim
				if(e>=c) { // Make sure name exists
					strncpy(buffer, c, e-c+1);
					buffer[e-c+1] = 0;

					// Get value
					while(*v && whitespace(v)) ++v;
					e = c = v;
					while(*e && !newline(e) && *e!=';' && *e!='#') ++e; // find end of name
					--e;
					while(whitespace(e)) --e; // trim
					if(e>=c) { // Make sure value exists
						Value value;
						value.m_source = (char*)malloc(e-c+2);
						strncpy(value.m_source, c, e-c+1);
						value.m_source[e-c+1] = 0;
						s->set(buffer, value);
						c = e;
					}
				} else printf("INI Warning: found '=' with no name\n");
			}
			else if(e>c+1) {
				// Just some text
				Value value;
				value.m_source = (char*)malloc(e-c+1);
				strncpy(value.m_source, c, e-c);
				value.m_source[e-c] = 0;
				s->m_values.push_back( KeyValue { 0, value } );
			}
			while(*c && !newline(c)) ++c;
		}
		while(whitespace(c) || newline(c)) ++c;
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

bool INIFile::Section::contains(const char* key) const { return m_map.contains(key); }
const INIFile::Value& INIFile::Section::operator[](const char* c) const { return get(c); }
void INIFile::Section::set(const char* c, const Value& v) {
	int index = m_map.get(c, -1);
	if(index<0) {
		m_map.insert(c, m_values.size());
		const char* key = m_map.find(c)->key;
		m_values.push_back( KeyValue{key, v} );
	}
	else m_values[index].value = v;
}
void INIFile::Section::add(const char* key, const Value& value) {
	auto item = m_map.find(key);
	if(item == m_map.end()) {
		m_map.insert(key, m_values.size());
		key = m_map.find(key)->key;
	}
	m_values.push_back(KeyValue{item->key, value});
}
const INIFile::Value& INIFile::Section::get(const char* c) const {
	int index = m_map.get(c, -1);
	if(index<0) return nullValue;
	return m_values[index].value;
}
INIFile::Value& INIFile::Section::operator[](const char* c) {
	int index = m_map.get(c, -1);
	if(index<0) {
		index = m_values.size();
		set(c, Value());
	}
	return m_values[index].value;
}

//// //// Value Functions //// ////
INIFile::Value::operator const char*() const {
	switch(m_type) {
	case SOURCE: return m_source;
	case STRING: return m_c;
	default: return 0;
	}
}
INIFile::Value::operator const char*() {
	switch(m_type) {
	case SOURCE: return m_source;
	case STRING: return m_c;
	default: setString(); return m_source;
	}
}
INIFile::Value::operator double() const {
	return (float)*this;
}
INIFile::Value::operator float() const {
	switch(m_type) {
	case SOURCE: return m_source? atof(m_source): 0;
	case STRING: return atof(m_c);
	case FLOAT:  return m_f;
	case BOOL:   return m_b? 1.0f: 0;
	case INTEGER:return m_i;
	case UNSIGNED: return m_u;
	default: return 0;
	}
}
INIFile::Value::operator bool() const {
	switch(m_type) {
	case SOURCE: return strcmp(m_source, "1")==0 || strcmp(m_source, "true")==0 || strcmp(m_source, "yes")==0;
	case STRING: return strcmp(m_source, "1")==0 || strcmp(m_source, "true")==0 || strcmp(m_source, "yes")==0;
	case FLOAT:  return m_f!=0;
	case BOOL:   return m_b;
	case UNSIGNED:
	case INTEGER:return m_i;
	default: return 0;
	}
}
INIFile::Value::operator int() const {
	switch(m_type) {
	case SOURCE: return m_source? atoi(m_source): 0;
	case STRING: return atoi(m_c);
	case FLOAT:  return m_f;
	case BOOL:   return m_b? 1: 0;
	case INTEGER:return m_i;
	case UNSIGNED: return m_i;
	default: return 0;
	}
}
INIFile::Value::operator unsigned() const {
	switch(m_type) {
	case SOURCE: return m_source? atoi(m_source): 0;
	case STRING: return atoi(m_c);
	case FLOAT:  return m_f;
	case BOOL:   return m_b? 1: 0;
	case INTEGER:return m_i;
	case UNSIGNED: return m_u;
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
	case UNSIGNED: sprintf(m_source, "%u", m_u); break;
	default: break;
	}
}


