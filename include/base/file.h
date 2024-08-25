#pragma once

#include <cstdio>

/** File loader class. This can be extended to use pak files etc */
namespace base {
	class File {
		friend class Directory;
		public:
		enum Mode { READ=1, WRITE=2, READWRITE=3, BUFFER=8 };
		File();
		File(const char* name, Mode mode=BUFFER);
		File(const char* name, char* data, size_t length);
		File(const File&) = delete;
		File(File&&) noexcept;
		~File();
		File& operator=(File&&) noexcept;
		operator const char*() const { return m_data; }
		operator bool() const { return m_data || m_file; }
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
		template<class T=char> T get()                              { T value=T(); read(value); return value; }
		template<class T=char> T get(T&& init)                      { T value=init; read(value); return value; }
		protected:
		int open();

		char* m_name;	// File name
		char* m_data;	// Buffered file data (read only)
		FILE* m_file;		
		size_t m_size;
		Mode m_mode;
	};
};

