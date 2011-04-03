#include "file.h"
#include <cstring>

using namespace base;

File::File() : m_name(0), m_path(0), m_data(0), m_file(0), m_size(0), m_mode(BUFFER), m_ref(0) {}
File::File(const char* name, Mode mode): m_name(name), m_path(0), m_data(0), m_file(0), m_size(0), m_mode(mode), m_ref(0) {
	open();
}
File::File(const File& f) : m_name(f.m_name), m_path(f.m_path), m_data(f.m_data), m_file(f.m_file), m_size(f.m_size), m_mode(f.m_mode), m_ref(f.m_ref) {
	(*m_ref) ++;
}
File::~File() {
	if(m_ref && *m_ref<=1) {
		if(m_file) fclose(m_file);
		if(m_data) delete [] m_data;
		delete m_ref;
	} else if(m_ref) (*m_ref)--;
}
int File::read(char* data, size_t length) {
	if(m_mode&1 && m_file) return fread(data, 1, length, m_file); 
	else return 0;
}
int File::write(char* data, size_t length) {
	if(m_mode&2 && m_file) return fwrite(data, 1, length, m_file); 
	else return 0;
}
int File::seek(int pos, int from) {
	if(m_mode&3 && m_file) return fseek(m_file, pos, from); 
	else return 1;
}
int File::find() {
	if(!s_paths.empty()) {
		for(std::list<Directory>::iterator i=s_paths.begin(); i!=s_paths.end(); i++) {
			if(i->find(*this)) return 1;
		}
	}
	return 0;
}
int File::open() {
	//get full file name
	char buffer[512];
	strcpy(buffer, m_path);
	strcat(buffer, "/");
	strcat(buffer, m_name);
	//open file
	m_file=fopen(buffer, m_mode&WRITE? "rw":"r");
	if(!m_file) return 0;
	fseek(m_file, 0, SEEK_END);
	m_size = ftell(m_file);
	rewind(m_file);

	//buffer
	if(m_mode == BUFFER) {
		m_data = new char[ m_size+1 ];
		m_size = fread(m_data, 1, m_size, m_file);
		fclose(m_file); m_file=0;
	}
	m_ref = new int(1);
	return 1;
}

std::list<Directory> File::s_paths;
void File::addPath(const Directory& d) {
	s_paths.push_back(d);
}
void File::clearPaths() {
	s_paths.clear();
}

int File::find(const char* name, char* path) {
	if(!s_paths.empty()) {
		for(std::list<Directory>::iterator i=s_paths.begin(); i!=s_paths.end(); i++) {
			if(i->find(name, path)) return 1;
		}
	}
	return 0;
}
//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
#ifdef WIN32
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif


Directory::Directory(const char* path) : m_directory(path), m_scanned(0) {
}
Directory::~Directory() {
}
int Directory::search() {
	//List all files in the directory
	#ifdef WIN32
	//Do this later
	//see ~/projects/doste/src/directory.cpp:59
	#else
	DIR* dp;
	struct stat st;
	struct dirent *dirp;
	char buffer[512];
	if((dp = opendir(m_directory))) {
		while((dirp = readdir(dp))) {
			m_files.push_back(DFile());
			DFile& f = m_files.back();
			strcpy(f.name, dirp->d_name);

			//is it a file or directory
			strcpy(buffer, m_directory);
			strcat(buffer, "/");
			strcat(buffer, dirp->d_name);
			stat(buffer, &st);
			if(S_ISDIR(st.st_mode)) {
				f.flags |= 0x10;
			}
			//extract extension
			for(const char* c = f.name; *c; c++) if(*c=='.') f.ext=c+1;
		}
	}
			
	#endif
	return 0;
}
bool Directory::contains( const char* file ) {
	if(m_scanned) {
		//is it in the list?
		for(iterator i=begin(); i!=end(); i++) {
			if(strcmp(i->name, file)==0) return true;
		}
	} else {
		//try to open it
		char buf[128];
		strcpy(buf, m_directory);
		strcat(buf, "/"); //linux specific
		strcat(buf, file);
		FILE* fp = fopen(buf, "r");
		if(fp) { fclose(fp); return true; }
	}
	return false;
}

int Directory::find(File& f) {
	if(contains(f.m_name)) {
		f.m_path = m_directory;
		return 1;
	}
	return 0;
}

int Directory::find(const char* name, char* path) {
	if(contains(name)) {
		strcpy(path, m_directory);
		strcat(path, "/");
		strcat(path, name);
		return 1;
	}
	return 0;
}

File Directory::open(const char* file, File::Mode mode) {
	char buf[128];
	strcpy(buf, m_directory);
	strcat(buf, "/"); //linux specific
	strcat(buf, file);
	return File(buf, mode);

}



