#pragma once

#include <vector>
#include <base/string.h>

namespace base {
class BinaryFileWriter {
	public:
	using BlockType = unsigned char;
	BinaryFileWriter(const char* file);
	BinaryFileWriter(const BinaryFileWriter&) = delete;
	~BinaryFileWriter();
	operator bool() const { return m_file; }
	void startBlock(BlockType type);
	void endBlock(BlockType type);
	void writeString(const char* s, int length);
	void write(const char* s);
	void write(const base::String& s);
	void write(bool value) { write(&value, 1, 1); }
	void write(const void* data, int stride, int count=1);
	template<class T> void write(const T& v) { 
		static_assert(!std::is_pointer<T>::value, "Can't write pointers to files");
		write(&v, sizeof(T), 1);
	}
	template<class T> void write(const T* data, int count) {
		write((void*)data, sizeof(T), count);
	}
	void write(const std::vector<bool>& bitset, int count); // special override for writing a packed bitset
	private:
	struct BlockInfo { BlockType type; unsigned int start; };
	std::vector<BlockInfo> m_blockStack;
	FILE* m_file;
};

class BinaryFileReader {
	public:
	BinaryFileReader(const char* file);
	BinaryFileReader(BinaryFileReader&) = delete;
	~BinaryFileReader();
	operator bool() const { return m_file; }
	bool valid();
	int nextBlock(bool subBlock=false);
	int currentBlock() const;
	bool insideBlock(int type) const;
	void endBlock(int id=0);
	bool readString(char* s, int limit);
	bool read(bool& b) { b=get<char>(); return 1; }
	bool read(void* data, int stride, int count=1);
	bool read(std::vector<bool>& bitset, int count); // special override for reading a packed bitset
	template<class T> bool read(const T& v) { return read((void*)&v, sizeof(T), 1); }
	template<class T> bool read(const T* v, int count) { return read((void*)v, sizeof(T), count); }
	template<class T> T get(const T& fallback = {}) {
		static_assert(!std::is_pointer<T>::value, "Can't read pointer types from file");
		T tmp = fallback; if(read(tmp)) return tmp;
		return fallback;
	}
	base::String get(const char* v);
	base::String get(const base::String& s) { return get(s.str()); }
	base::String getString() { return get(""); }
	private:
	struct BlockInfo { int type; size_t start; size_t end; };
	std::vector<BlockInfo> m_blockStack;
	FILE* m_file;
};
}



/*
 *  2
 *  ---
 *  ---
 *  4
 *  ---
 *  --
 *  ---
 *  5
 *  ---
 *    6
 *    --
 *    7
 *    ---
 *  9
 *  ---
 *  ---
 *
 *
 *
 */
