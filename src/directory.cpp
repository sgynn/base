
#include "base/directory.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>


#ifdef WIN32
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

using namespace base;

Directory::Directory(const char* path) {
	clean(path, m_path);
}
Directory::~Directory() {
}

/** Get a clean path */
int Directory::clean(const char* in, char* out) {
	int k=0;
	for(const char* c=in; *c; c++) {
		if(strncmp(c, "//", 2)==0) continue ; // repeated slashes
		else if((c==in||*(c-1)=='/') && strncmp(c, "./", 2)==0) c++; // no change
		else if(strncmp(c, "/.", 3)==0) c++; // ending in /.
		else if(strncmp(c, "/../", 4)==0 || strncmp(c, "/..", 4)==0) { //up
			if((k==2 && strncmp(out, "..", 2)) || (k>2 && strncmp(&out[k-3], "/..", 3))) {
				//remove segment
				while(k>1 && out[k-1]!='/') k--;
				c+=2;
			} else out[k++] = *c;
		} else out[k++] = *c;
	}
	out[k]=0;
	return strlen(out);
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
			// onvert to char* from wchar_t
			for(int i=0; i<64; i++) {
				file.name[i] = (char)findFileData.cFileName[i];
				if(f[i]==0) break;
			}

			// Is it a directory?
			if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) file.type=DIRECTORY;
			else file.type=Directory::FILE;

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
			sprintf(buffer, "%s/%s\n", m_path, dirp->d_name);
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
	//m_files.sort(SortFiles);
	return 0;
}


bool Directory::contains( const char* file ) {
	if(m_files.empty()) scan();
	for(iterator i=m_files.begin(); i!=m_files.end(); i++) {
		if(strcmp(i->name, file)==0) return true;
	}
	return false;
}


