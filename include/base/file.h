#ifndef _BASE_FILE_
#define _BASE_FILE_

#include <vector>
#include <cstdio>

// Macro to convert filename into a found file based on path lookup
#define FindFile(filename) char path_##filename[128]; if(base::File::find(filename, path_##filename)) filename = path_##filename;

/** File loader class. This can be extended to use pak files etc */
namespace base {
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
		static void addPath(const char* path);
		static void clearPaths();
		static int find(const char* name, char* path); //lightweigt version
		private:
		static std::vector<const char*> s_paths;
	};
};

#endif

