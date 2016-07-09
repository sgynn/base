
#include "base/directory.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>


#ifdef WIN32
#include <windows.h>
#include <direct.h>
#define PATH_MAX 2048
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

// FIle list sorting functor //
namespace base { struct SortFiles {
	bool operator()(const Directory::File& a, const Directory::File& b) const {
		if(a.type!=b.type) return a.type>b.type; // List folders at the top?

		// Case insensitive matching
		register unsigned char* s0 = (unsigned char*) a.name;
		register unsigned char* s1 = (unsigned char*) b.name;
		unsigned char c0, c1;
		const unsigned char u = 'a'-'A';
		do {
			c0 = (unsigned char) (*s0>='a' && *s0<='z'? *s0-u: *s0); ++s0;
			c1 = (unsigned char) (*s1>='a' && *s1<='z'? *s1-u: *s1); ++s1;
			if(c1==0) return c0<c1;
		} while(c0 == c1);
		return c0<c1;
	}
};};

using namespace base;

Directory::Directory(const char* path) {
	clean(path, m_path);
}
Directory::~Directory() {
}

/** Get a clean path */
int Directory::clean(const char* in, char* out, int lim) {
	// remove any backslashes
	if(strchr(in, '\\')) {
		size_t l = strlen(in)+1;
		char* tmp = new char[l];
		memcpy(tmp, in, l);
		for(size_t i=0; i<l; ++i) if(tmp[i]=='\\') tmp[i]='/';
		int r = clean(tmp, out, lim);
		delete [] tmp;
		return r;
	}

	int k=0;
	for(const char* c=in; *c; c++) {
		if(strncmp(c, "//", 2)==0) continue ; // repeated slashes
		else if((c==in||*(c-1)=='/') && strncmp(c, "./", 2)==0) c++; // no change
		else if(strncmp(c, "/.", 3)==0) c++; // ending in /.
		else if(strncmp(c, "/../", 4)==0 || strncmp(c, "/..", 4)==0) { //up
			if((k==2 && strncmp(out, "..", 2)) || (k>2 && strncmp(&out[k-3], "/..", 3))) {
				//remove segment
				while(k>0 && out[k-1]!='/') k--;
				c+=2;
			} else out[k++] = *c;
		} else out[k++] = *c;
	}
	out[k]=0;
	if(k==0) strcpy(out, ".");
	return strlen(out);
}

bool Directory::isRelative(const char* path) {
	#ifdef WIN32
	return path[1] != ':';
	#else
	return path[0] != '/';
	#endif
}

int Directory::getFullPath(const char* in, char* out, int lim) {
	if(lim<=0) lim = PATH_MAX;
	if(isRelative(in)) {
		// Get working directory
		char buffer[PATH_MAX];
		#ifdef WIN32
		if(!_getcwd(buffer, PATH_MAX)) return 0;
		#else
		if(!getcwd(buffer, PATH_MAX)) return 0;
		#endif

		// Append in
		size_t l = strlen(buffer);
		buffer[l] = '/';
		strncpy(buffer+l+1, in, lim-l-2);
		return clean(buffer, out, lim);
	}
	return clean(in, out, lim);
}

int Directory::getRelativePath(const char* in, char* out, int lim) {
	if(lim<=0) lim = PATH_MAX;
	if(!isRelative(in)) {
		// Get working directory
		char buffer[PATH_MAX];
		#ifdef WIN32
		if(!_getcwd(buffer, PATH_MAX)) return 0;
		//if(buffer[0] != in[0]) return clean(in, out, lim);	// Impossible
		#else
		if(!getcwd(buffer, PATH_MAX)) return 0;
		#endif
		// match ...
		int m=0, up=0;
		while(buffer[m] == in[m]) ++m;
		while(m>0 && (in[m]=='/' || in[m]=='\\')) ++m;
		for(char* c = buffer+m; *c; ++c) if(*c=='/' || *c=='\\') ++up;
		// Build
		int k = 2;
		if(up==0) strcpy(buffer, "./");
		else {
			for(int i=0; i<up; ++i) strcpy(buffer+i*3, "../");
			k = up*3;
		}
		strncpy(buffer+k, in+m, lim-k);
		return clean(buffer, out, lim);
	}
	return clean(in, out, lim);
}

/** Scan directory for files */
int Directory::scan() {
	m_files.clear();

	#ifdef WIN32 //// //// //// //// WINDOWS //// //// //// ////

	int tLen = strlen(m_path);
	#ifdef UNICODE // Using unicode?
	wchar_t *dir = new wchar_t[tLen+3];
	mbstowcs(dir, m_path, tLen+1);
	#else
	char *dir = new char[tLen+3];
	strcpy(dir, m_path);
	#endif
	//this string must be a search pattern, so add /* to path
	dir[tLen+0] = '/';	
	dir[tLen+1] = '*';
	dir[tLen+2] = 0;
	// Search
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(dir, &findFileData);	
	if(hFind  != INVALID_HANDLE_VALUE) {
		do {
			File file;
			// Convert to char* from wchar_t
			TCHAR* wName = findFileData.cFileName;
			for(int i=0; i<64 && (!i||wName[i-1]); i++) file.name[i] = (char) wName[i];

			// Is it a directory?
			if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) file.type=DIRECTORY;
			else file.type = Directory::FILE;

			//extract extension
			for(file.ext=0; file.name[file.ext]; ++file.ext) {
				if(file.ext && file.name[file.ext-1]=='.') break;
			}
			m_files.push_back(file);
		} while(FindNextFile(hFind, &findFileData));
		FindClose(hFind);
	}
	delete [] dir;

	#else //// //// //// //// LINUX //// //// //// ////

	DIR* dp;
	struct stat st;
	struct dirent *dirp;
	char buffer[1024];
	if((dp = opendir(m_path))) {
		while((dirp = readdir(dp))) {
			File file;
			strcpy(file.name, dirp->d_name);

			//is it a file or directory
			sprintf(buffer, "%s/%s", m_path, dirp->d_name);
			stat(buffer, &st);
			if(S_ISDIR(st.st_mode)) file.type = DIRECTORY;
			else file.type = Directory::FILE;

			//extract extension
			for(file.ext=0; file.name[file.ext]; ++file.ext) {
				if(file.ext && file.name[file.ext-1]=='.') break;
			}
			m_files.push_back(file);
		}
	}
	#endif
	std::sort(m_files.begin(), m_files.end(), SortFiles());
	return 0;
}


bool Directory::contains( const char* file ) {
	if(m_files.empty()) scan();
	for(iterator i=m_files.begin(); i!=m_files.end(); i++) {
		if(strcmp(i->name, file)==0) return true;
	}
	return false;
}


