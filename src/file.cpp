#include "base/file.h"
#include <cstdlib>
#include <cstring>

using namespace base;

File::File() : m_name(0), m_path(0), m_data(0), m_file(0), m_size(0), m_mode(BUFFER) {}
File::File(const char* name, Mode mode): m_name(0), m_path(0), m_data(0), m_file(0), m_size(0), m_mode(mode) {
	m_name = m_path = strdup(name);
	if(mode!=WRITE) find();
	open();
}
File::File(File&& f) : m_name(f.m_name), m_path(f.m_path), m_data(f.m_data), m_file(f.m_file), m_size(f.m_size), m_mode(f.m_mode) {
	f.m_name = 0;
	f.m_path = 0;
	f.m_data = 0;
	f.m_file = 0;
}
File::~File() {
	if(m_file) fclose(m_file);
	if(m_data) delete [] m_data;
	if(m_path != m_name) delete [] m_path;
	free(m_name);
}
int File::read(char* data, size_t length) {
	if(m_mode&1 && m_file) return fread(data, 1, length, m_file); 
	else return 0;
}
int File::write(const char* data, size_t length) {
	if(m_mode&2 && m_file) return fwrite(data, 1, length, m_file); 
	else return 0;
}
int File::seek(int pos, int from) {
	if(m_mode&3 && m_file) return fseek(m_file, pos, from); 
	else return 1;
}
size_t File::tell() const {
	if(m_mode&3 && m_file) return ftell(m_file);
	else return 0;
}
bool File::eof() const {
	if(m_mode&3 && m_file) return feof(m_file);
	else return true;
}

int File::find() {
	if(m_path == m_name) m_path = new char[512];	
	if(!find(m_name, m_path)) {
		delete [] m_path;
		m_path = m_name;
		return 0;
	}
	return 1;
}
int File::open() {
	switch(m_mode) {
	case READ:
	case BUFFER:
		m_file = fopen(m_path, "rb");
		if(!m_file) return 0;
		fseek(m_file, 0, SEEK_END);
		m_size = ftell(m_file);
		rewind(m_file);
		break;
	case WRITE:
		m_file = fopen(m_path, "wb");
		break;
	case READWRITE:
		m_file = fopen(m_path, "rwb");
		break;
	}
	
	//buffer
	if(m_mode == BUFFER) {
		m_data = new char[ m_size+1 ];
		m_size = fread(m_data, 1, m_size, m_file);
		m_data[m_size] = 0;
		fclose(m_file);
		m_file = 0;
	}
	return 1;
}

std::vector<const char*> File::s_paths;
void File::addPath(const char* d) { s_paths.push_back(d); }
void File::clearPaths() { s_paths.clear(); }
int File::find(const char* name, char* path) {
	for(unsigned i=0; i<s_paths.size(); ++i) {
		sprintf(path, "%s/%s", s_paths[i], name);
		FILE* fp = fopen(path, "r");
		if(fp) { fclose(fp); return 1; }
	}
	return 0;
}

