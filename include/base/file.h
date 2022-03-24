#pragma once

#include <vector>
#include <cstdio>

/** File loader class. This can be extended to use pak files etc */
namespace base {
	class File {
		friend class Directory;
		public:
		enum Mode { READ=1, WRITE=2, READWRITE=3, BUFFER=8 };
		File();
		File(const char* name, Mode mode=BUFFER);
		File(const File&) = delete;
		File(File&&);
		~File();
		const char* data() const { return m_data; }
		bool isOpen() const { return m_file != 0; }
		size_t size() const { return m_size; }
		size_t tell() const;
		bool eof() const;
		int seek(int pos, int from=SEEK_SET);
		int read(char* data, size_t length);
		int write(const char* data, size_t length);
		template<class T> int write(const T& value)                 { return write((const char*)&value, sizeof(T)); }
		template<class T> int writeArray(const T* value, int count) { return write((const char*)value, sizeof(T)*count); }
		template<class T> bool read(T& value)                       { return read((char*)&value, sizeof(value)) == sizeof(T); }
		template<class T> bool readArray(T* value, int count)       { return read((char*)value, sizeof(T)*count) == sizeof(T)*count; }
		template<class T=char> T get()                              { T value; read(value); return value; }
		protected:
		int open();
		int find();

		char* m_name;	// File name
		char* m_path;	// Full path

		char* m_data;	// Buffered file data (read only)
		FILE* m_file;		
		size_t m_size;
		Mode m_mode;

		// Mechanism to add search paths
		public:
		static void addPath(const char* path);
		static void clearPaths();
		static int find(const char* name, char* path); //lightweigt version
		private:
		static std::vector<const char*> s_paths;
	};
};

