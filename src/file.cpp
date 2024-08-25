#include "base/file.h"
#include <cstdlib>
#include <cstring>

using namespace base;

File::File() : m_name(0), m_data(0), m_file(0), m_size(0), m_mode(BUFFER) {}
File::File(const char* name, Mode mode): m_name(0), m_data(0), m_file(0), m_size(0), m_mode(mode) {
	m_name = strdup(name);
	open();
}
File::File(const char* name, char* data, size_t length) : m_data(data), m_file(0), m_size(length), m_mode(BUFFER) {
	m_name = strdup(name);
}
File::File(File&& f) noexcept : m_name(f.m_name), m_data(f.m_data), m_file(f.m_file), m_size(f.m_size), m_mode(f.m_mode) {
	f.m_name = 0;
	f.m_data = 0;
	f.m_file = 0;
}
File::~File() {
	if(m_file) fclose(m_file);
	if(m_data) delete [] m_data;
	free(m_name);
}
File& File::operator=(File&& f) noexcept {
	if(m_file) fclose(m_file);
	if(m_data) delete [] m_data;
	free(m_name);
	m_file = f.m_file;
	m_data = f.m_data;
	m_file = f.m_file;
	m_size = f.m_size;
	f.m_file = 0;
	f.m_data = 0;
	f.m_file = 0;
	return *this;
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

int File::open() {
	switch(m_mode) {
	case READ:
	case BUFFER:
		m_file = fopen(m_name, "rb");
		if(!m_file) return 0;
		fseek(m_file, 0, SEEK_END);
		m_size = ftell(m_file);
		rewind(m_file);
		break;
	case WRITE:
		m_file = fopen(m_name, "wb");
		break;
	case READWRITE:
		m_file = fopen(m_name, "rwb");
		break;
	}
	
	//buffer
	if(m_mode == BUFFER) {
		m_data = new char[m_size+1];
		m_size = fread(m_data, 1, m_size, m_file);
		m_data[m_size] = 0;
		fclose(m_file);
		m_file = 0;
	}
	return 1;
}

