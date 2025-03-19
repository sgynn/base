#include <stdexcept>
#include <cstdio>
#include <string.h>
#include <base/binaryfile.h>

using namespace base;

BinaryFileWriter::BinaryFileWriter(const char* file) {
	m_file = fopen(file, "wb");
}

BinaryFileWriter::~BinaryFileWriter() {
	if(m_file) {
		while(!m_blockStack.empty()) {
			printf("Error: File block %d not ended\n", m_blockStack.back().type);
			endBlock(m_blockStack.back().type);
		}
		fclose(m_file);
	}
}

void BinaryFileWriter::startBlock(BlockType type) {
	// Block size does not include block header
	if(type==0) throw new std::invalid_argument("Invalid block id");
	int zero = 0;
	fwrite(&type, sizeof(type), 1, m_file);
	fwrite(&zero, 4, 1, m_file);
	m_blockStack.push_back({ type, (unsigned int)ftell(m_file) - 4 });
}

void BinaryFileWriter::endBlock(BlockType type) {
	if(m_blockStack.empty() || m_blockStack.back().type != type) {
		throw new std::invalid_argument("Incorrect end block id");
	}

	unsigned int length = ftell(m_file) - m_blockStack.back().start - 4;
	fseek(m_file, m_blockStack.back().start, SEEK_SET);
	fwrite(&length, 4, 1, m_file);
	fseek(m_file, 0, SEEK_END);

	m_blockStack.pop_back();
}

void BinaryFileWriter::write(const void* data, int stride, int count) {
	fwrite(data, stride, count, m_file);
}

void BinaryFileWriter::writeString(const char* string, int length) {
	if(length >= 0x7fff) throw new std::invalid_argument("String too long");
	if(length < 128) fputc(length, m_file);
	else {
		fputc(0x80 | length>>8, m_file);
		fputc(length&0xff, m_file);
	}
	fwrite(string, 1, length, m_file);
}

void BinaryFileWriter::write(const char* string) {
	writeString(string, strlen(string));
}

void BinaryFileWriter::write(const base::String& string) {
	writeString(string, string.length());
}

void BinaryFileWriter::write(const std::vector<bool>& bitset, int count) {
	char c = 0;
	for(int i=0; i<count; ++i) {
		if(bitset[i]) c |= 1<<(i&7);
		if((i&7)==7) { fputc(c, m_file); c=0; }
	}
	if(count&7) fputc(c, m_file);
}

// =============================================================== //

BinaryFileReader::BinaryFileReader(const char* file) {
	m_file = fopen(file, "rb");
}

BinaryFileReader::~BinaryFileReader() {
	if(m_file) fclose(m_file);
}

bool BinaryFileReader::valid() {
	if(!m_file || feof(m_file)) return false;
	if(m_blockStack.empty()) return true;
	return ftell(m_file) < (int)m_blockStack.back().end;
}

int BinaryFileReader::nextBlock(bool subBlock) {
	// Are we inside a block - skip to end
	if(!m_blockStack.empty()) {
		size_t pos = ftell(m_file);
		size_t end = m_blockStack.back().end;
		if(pos>=end && subBlock) return 0;
		if(pos!=end && !subBlock) fseek(m_file, end, SEEK_SET);
		if(!subBlock) m_blockStack.pop_back();
	}

	unsigned int length = 0;
	int type = fgetc(m_file);
	fread(&length, 4, 1, m_file);
	if(feof(m_file)) return 0;
	unsigned int start = ftell(m_file);
	m_blockStack.push_back({type, start, start + length});
	return type;
}

void BinaryFileReader::endBlock(int type) {
	// allow exiting multiple blocks, 0=current 
	if(m_blockStack.empty()) return;
	if(type && currentBlock() != type) {
		if(!insideBlock(type)) {
			printf("Error: Cannot end file block %d\n", type);
			return;
		}
		while(m_blockStack.back().type != type) m_blockStack.pop_back();
	}

	size_t pos = ftell(m_file);
	size_t end = m_blockStack.back().end;
	if(pos!=end) fseek(m_file, end, SEEK_SET);
	m_blockStack.pop_back();
}

int BinaryFileReader::currentBlock() const {
	return m_blockStack.empty()? 0: m_blockStack.back().type;
}

bool BinaryFileReader::insideBlock(int type) const {
	for(const BlockInfo& b: m_blockStack) if(b.type == type) return true;
	return false;
}

bool BinaryFileReader::readString(char* s, int lim) {
	int len = fgetc(m_file);
	if(len&0x80) len = (len&0x7f) << 8 | fgetc(m_file);
	if(len == 0) { s[0] = 0; return true; } // Empty string
	if(--lim > len) lim = len;
	if(read(s, lim)) {
		if(lim < len) fseek(m_file, len-lim, SEEK_CUR); // Truncated
		s[lim] = 0;
		return true;
	}
	return false;
}

base::String BinaryFileReader::get(const char* fallback) {
	int len = fgetc(m_file);
	if(len&0x80) len = (len&0x7f) << 8 | fgetc(m_file);
	if(len == 0) return base::String();
	base::String string;
	char* buffer = string.allocateUnsafe(len+1);
	if(read(buffer, len)) {
		buffer[len] = 0;
		return string;
	}
	return base::String();
}


bool BinaryFileReader::read(void* data, int stride, int count) {
	if(!m_blockStack.empty() && ftell(m_file) + stride*count > (int)m_blockStack.back().end) return false;
	return fread(data, stride, count, m_file) == (size_t)count;
}

bool BinaryFileReader::read(std::vector<bool>& bitset, int count) {
	int i = 0, end = count;
	bitset.resize(count);
	while(i<count) {
		char c = fgetc(m_file);
		int e = std::min(8, end);
		for(int k=0; k<e; ++k) {
			bitset[i+k] = c & 1<<k;
		}
		end -= 8;
		i+=8;
	}
	return valid();
}

