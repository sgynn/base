#ifndef _FILE_
#define _FILE_

#include <list>
#include <cstdio>

/** File loader class. This can be extended to use pak files etc */
namespace base {
class Directory;
class File {
	friend class Directory;
	public:
	enum Mode { READ=1, WRITE=2, READWRITE=3, BUFFER=8 };
	File();
	File(const char* name, Mode mode=BUFFER);
	File(const File&);
	~File();
	const char* data() const { return m_data; }
	size_t size() const { return m_size; }
	int read(char* data, size_t length);
	int write(char* data, size_t length);
	int seek(int pos, int from=SEEK_SET);
	protected:
	int open();
	int find();
	const char* m_name;	//File name
	const char* m_path;

	char* m_data;		//buffered file data (read only)
	FILE* m_file;		
	size_t m_size;
	Mode m_mode;
	int* m_ref;

	// Mechanism to add search paths
	public:
	static void addPath(const Directory& path);
	static void clearPaths();
	static int find(const char* name, char* path); //lightweigt version
	private:
	static std::list<Directory> s_paths;
};

/** Use this to list files in a directory. Extend directory for pak files etc. */
class Directory {
	public:
	Directory(const char* path);
	~Directory();
	int search(); // scan directory
	int find(File& file);
	int find(const char* name, char* path);
	bool contains(const char* file);
	File open(const char* file, File::Mode mode);

	//Needs an iterator, perhaps with extension, and type mask (dir,file)
	struct DFile { char name[64]; const char* ext; int flags; };
	typedef std::list<DFile>::const_iterator iterator;
	iterator begin() const { return m_files.begin(); }
	iterator end() const { return m_files.end(); }

	protected:
	const char* m_directory;
	bool m_scanned;
	std::list<DFile> m_files;

};
};

#endif

