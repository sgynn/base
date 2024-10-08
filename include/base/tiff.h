#pragma once

#include <base/image.h>

namespace base {

/** Only need these static functions for API */
class Tiff {
	public:
	static Image load(const char* filename);
	static bool save(const Image& image, const char* filename);
};


/** Allow streaming - only uncompressed images */
class TiffStream {
	public:
	typedef unsigned uint;
	enum Mode { READ=1, WRITE=2, READWRITE=3 };
	static TiffStream* openStream(const char* filename, Mode mode=READ);
	static TiffStream* createStream(const char* filename, int width, int height, Image::Format format, Mode mode=WRITE, const void* data=0, size_t length=0);
	bool    good() const     { return m_stream!=0; }
	uint    bpp()  const     { return m_bitsPerPixel; }
	uint    width() const    { return m_width; }
	uint    height() const   { return m_height; }
	uint    channels() const { return m_samplesPerPixel; }
	uint    sampleFormat() const { return m_sampleFormat; }

	size_t getPixel(int x, int y, void* data) const;
	size_t readBlock(int x, int y, int width, int height, void* data) const;

	size_t setPixel(int x, int y, void* data);
	size_t writeBlock(int x, int y, int width, int height, const void* data);

	~TiffStream();

	private:
	friend class Tiff;
	FILE* m_stream = nullptr;
	uint  m_width=0, m_height=0;
	unsigned short m_bitsPerSample[4] = {0,0,0,0};
	uint  m_bitsPerPixel = 0;
	uint  m_samplesPerPixel=0;
	uint  m_rowsPerStrip=0;
	uint  m_sampleFormat=1;
	uint  m_stripCount=0;
	uint* m_stripOffsets=nullptr;

	size_t getAddress(int x, int y) const;
};
}

