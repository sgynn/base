#include "base/inifile.h"
#include <cstdio>
#include <cstdlib>

using namespace base;

INIFile::INIFile() {
}
INIFile::~INIFile() {
}

INIFile::Section* INIFile::getSection(const char* name) {
	if(m_sections.contains(name)) return m_sections[name];
	else return m_sections.insert(name, new Section(name));
}

bool INIFile::loadFile(const char* filename) {
	FILE* fp = fopen(filename, "r");
	if(!fp) return false;
	fseek(fp, 0, SEEK_END);
	size_t length = ftell(fp)+1;
	rewind(fp);
	char* data = new char[length];
	length = fread(data, 1, length-1, fp);
	data[length-1] = 0;
	bool r = setData(data, length);
	delete [] data;
	return r;
}
bool INIFile::saveFile(const char* filename) {
	FILE* fp = fopen(filename, "w");
	if(!fp) return false;
	for(HashMap<Section*>::iterator i=m_sections.begin(); i!=m_sections.end(); i++) {
		fprintf(fp, "[%s]\n", i.key());
		Section* s = *i;
		for(HashMap<const char*>::iterator j = s->m_data.begin(); j!=s->m_data.end(); j++) {
			fprintf(fp, "%s = %s\n", j.key(), *j);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
	return true;
}

bool INIFile::setData(const char* data, size_t length) {
	char buf[128], buf2[128]; char* i;
	const char* c = data;
	Section* s = 0;
	while(*c) {
		if(*c=='#' || *c==';') while(*c) { if(*(c++)=='\n') break; } // comment
		//Section header
		else if(*c == '[') {
			i=buf; c++;
			while(*c && *c!=']') { *(i++)=*c; c++; }
			*i = 0;
			s = getSection(buf);
			while(*c && *(c++)!='\n'); //next line

		} else { //Attributes
			i = buf;
			while(*c && *c!='=' && *c!='\n' && *c != ';') *(i++) = *(c++);
			*i = 0;
			while(i>=buf && (*i==0 || *i==' ' || *i=='\t')) *(i--) = 0; //rtrim
			//value
			if(*c=='=') {
				++c;
				//lift value
				i = buf2;
				while(*c && (*c==' ' || *c=='\t')) c++; //ltrim
				while(*c && *c!='\n' && *c != ';') *(i++) = *(c++);
				*i = 0;
				while(i>=buf2 && (*i==0 || *i==' ' || *i=='\t')) *(i--) = 0; //rtrim
				//Add attribute
				s->setValue(buf, buf2);
			} else if(buf[0]) {
				printf("Warning: No value for '%s' in section %s\n", buf, s->name());
			}
			while(*c && *(c++)!='\n'); //next line
		}
	}
	return true;
}

//// //// Section functions //// ////
INIFile::Section::Section(const char* name) {
	strcpy(m_name, name);
}
INIFile::Section::~Section() {
	for(HashMap<const char*>::iterator i=m_data.begin(); i!=m_data.end(); i++) delete [] *i;
}
void INIFile::Section::setValue(const char* name, const char* value) {
	if(m_data.contains(name)) delete [] m_data[name];
	m_data[name] = strdup(value);
}
void INIFile::Section::setValue(const char* name, int value) {
	char buf[32]; sprintf(buf, "%d", value);
	setValue(name, buf);
}
void INIFile::Section::setValue(const char* name, unsigned int value) {
	char buf[32]; sprintf(buf, "%u", value);
	setValue(name, buf);
}
void INIFile::Section::setValue(const char* name, float value) {
	char buf[32]; sprintf(buf, "%f", value);
	int l = strlen(buf);
	while(--l>1) if(buf[l]=='0' && buf[l-1]!='.') buf[l]=0; //trim trailing zeros
	setValue(name, buf);
}
const char* INIFile::Section::getValue(const char* name, const char* value) const {
	if(m_data.contains(name)) return m_data[name];
	else return value;
}
int INIFile::Section::getValue(const char* name, int value) const {
	const char* v = getValue(name);
	if(v[0]) return atoi(v);
	else return value;
}
unsigned int INIFile::Section::getValue(const char* name, unsigned int value) const {
	const char* v = getValue(name);
	if(v[0]) return atoi(v);
	else return value;
}
float INIFile::Section::getValue(const char* name, float value) const {
	const char* v = getValue(name);
	if(v[0]) return atof(v);
	else return value;
}





