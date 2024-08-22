#include <base/png.h>
#include <cstdio>
#include <cstring>

#define MINIZ_HEADER_FILE_ONLY
#include "miniz.c"

using namespace base;
using byte = unsigned char;

static constexpr unsigned char magic[8] = {137, 80, 78, 71, 13, 10, 26, 10};
enum class PNGColour : byte { Lum=0, RGB=2, Indexed=3, LumAlpha=4, RGBA=6 };
struct PNGChunk {
	unsigned length;
	char type[4];
	const byte* data;
};

struct PNGHeader {
	unsigned width;
	unsigned height;
	byte bitDepth;
	PNGColour colourType;
	byte compression;
	byte filter;
	byte interlace;
};


// Offset from left pixel
static void readSubFilter(byte* data, unsigned length, int depth) {
	int bpp = (depth+7) >> 3;
	data += bpp;
	for(unsigned i=bpp; i<length; ++i) {
		*data = ((int)*data + (int)*(data-bpp)) & 0xff;
		++data;
	}
}
// Offset from pixel above
static void readUpFilter(byte* data, unsigned length, const byte* prev) {
	for(unsigned i=0; i<length; ++i) {
		data[i] = (data[i] + prev[i]) & 0xff;
	}
}
// Offset from average of upper and left pixels
static void readAvgFilter(byte* data, unsigned length, int depth, const byte* prev) {
	int bpp = (depth+7) >> 3;
	for(int i=0; i<bpp; ++i) { // First pixel - only above
		*data = ((int)*data + (int)*prev++ / 2) & 0xff;
		++data;
	}
	for(unsigned i=1; i<length; ++i) {
		*data = ((int)*data + ((int)*prev++ + *(data-bpp)) / 2) & 0xff;
		++data;
	}
}
// Complicated thing based on upper, upper left amd left pixels
static void readPaethFilter(byte* data, unsigned length, int depth, const byte* prev) {
	int bpp = (depth+7) >> 3;
	const byte* end = data + length;
	for(int i=0; i<bpp; ++i) {
		*data = ((int)*prev++ + *data) & 0xff;
		++data;
	}
	while(data<end) {
		int c = *(prev - bpp);
		int a = *(data - bpp);
		int b = *prev++;
		int p = b - c;
		int pc = a - c;
		int pa = abs(p);
		int pb = abs(pc);
		pc = abs(p + pc);
		// Find predictor
		if(pb < pa) pa = pb, a = b;
		if(pc < pa) a = c;
		// Calculate pixel
		*data = (a + *data) & 0xff;
		++data;
	}
}

template<class ReadFunc>
Image readPNGFile(ReadFunc&& read) {
	// Check signiture
	const byte* signiture = read(8);
	if(memcmp(signiture, magic, 8) != 0) {
		return Image();
	}

	static auto parseUInt = [](const byte* data)->unsigned {
		return data[0]<<24 | data[1] << 16 | data[2] << 8 | data[3];
	};

	auto readChunk = [&read](PNGChunk& chunk) {
		chunk.length = parseUInt(read(4));
		memcpy(chunk.type, read(4), 4);
		chunk.data = chunk.length? read(chunk.length): nullptr;
		// crc check - using crc32 from miniz
		unsigned crc = parseUInt(read(4));
		unsigned crc1 = crc32(0, (byte*)chunk.type, 4);
		if(chunk.data) crc1 = crc32(crc1, chunk.data, chunk.length);
		if(crc != crc1) printf("CRC check failed on block %.4s\n", chunk.type);
	};

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	if(inflateInit(&stream) != Z_OK) return Image();
	byte* rowBuffer = nullptr;
	unsigned rowSize = 0;
	unsigned bitsPerPixel = 0;

	PNGHeader header = {0,0,0,PNGColour::Lum,0,0,0};
	PNGChunk chunk;
	chunk.data = nullptr;
	byte* pixels = nullptr;
	byte* palette = nullptr;
	Image::Format format = Image::INVALID;

	readChunk(chunk);
	while(chunk.type[0]) {
		// do something with chunk
		if(memcmp(chunk.type, "IHDR", 4)==0) {
			header.width = parseUInt(chunk.data);
			header.height = parseUInt(chunk.data+4);
			header.bitDepth = chunk.data[8];
			header.colourType = (PNGColour)chunk.data[9];
			header.compression = chunk.data[10];
			header.filter = chunk.data[11];
			header.interlace = chunk.data[12];
			
			switch((PNGColour)header.colourType) {
			case PNGColour::Lum:
				if(header.bitDepth == 8) format = Image::R8;
				else if(header.bitDepth == 16) format = Image::R16;
				bitsPerPixel = header.bitDepth;
				break;
			case PNGColour::Indexed:
				bitsPerPixel = header.bitDepth;
				format = Image::RGB8;
				break;
			case PNGColour::LumAlpha: 
				if(header.bitDepth == 8) format = Image::RG8;
				bitsPerPixel = header.bitDepth * 2;
				break;
			case PNGColour::RGB:
				if(header.bitDepth == 8) format = Image::RGB8;
				bitsPerPixel = header.bitDepth * 3;
				break;
			case PNGColour::RGBA:
				if(header.bitDepth == 8) format = Image::RGBA8;
				bitsPerPixel = header.bitDepth * 4;
				break;
			}
			if(format == Image::INVALID) break;
			rowSize = header.width * bitsPerPixel/8;
			rowBuffer = new byte[rowSize + 1];
			pixels = new byte[rowSize * header.height];
		}
		else if(memcmp(chunk.type, "IDAT", 4)==0) { // Pixel data
			if(stream.total_out) break; // Error - all IDAT blocks are handles together
			bool fail = false;
			stream.next_in = chunk.data;
			stream.avail_in = chunk.length;

			for(unsigned row = 0; row < header.height; ++row) {
				stream.avail_out = rowSize+1;
				stream.next_out = rowBuffer;

				while(stream.avail_out) {
					int r = inflate(&stream, Z_PARTIAL_FLUSH);

					if(r==Z_BUF_ERROR && stream.avail_out > 0 && stream.avail_in==0) {
						readChunk(chunk);
						if(memcmp(chunk.type, "IDAT", 4) !=0 ) { fail=true; break; }
						stream.next_in = chunk.data;
						stream.avail_in = chunk.length;
						continue;
					}

					if(r!=Z_OK && r!=Z_STREAM_END) {
						printf("Inflate fail %d\n", r);
						fail = true;
						break;
					}
				}
				if(fail) break;
				
				// Filter
				switch(rowBuffer[0]) {
				case 0: break;
				case 1: readSubFilter(rowBuffer+1, rowSize, bitsPerPixel); break;
				case 2: readUpFilter(rowBuffer+1, rowSize, pixels+(row-1)*rowSize); break;
				case 3: readAvgFilter(rowBuffer+1, rowSize, bitsPerPixel, pixels+(row-1)*rowSize); break;
				case 4: readPaethFilter(rowBuffer+1, rowSize, bitsPerPixel, pixels+(row-1)*rowSize); break;
				default: printf("Bad filter %d\n", rowBuffer[0]); fail=true; break;
				}
				memcpy(pixels + rowSize * row, rowBuffer+1, rowSize);
			}
			if(fail) { format = Image::INVALID; break; }
		}
		else if(memcmp(chunk.type, "PLTE", 4)==0) { // Palette for indexed images
			if(header.colourType == PNGColour::Indexed) {
				palette = new byte[chunk.length];
				memcpy(palette, chunk.data, chunk.length);
			}
		}
		else if(memcmp(chunk.type, "IEND", 4)==0) {
			break;
		}
		readChunk(chunk);
	}
	delete [] rowBuffer;
	inflateEnd(&stream);

	// Convert Indexed to RGB8 image
	if(palette) {
		unsigned count = header.width * header.height;
		byte* index = pixels;
		pixels = new byte[count * 3];
		if(header.bitDepth == 8) {
			for(unsigned i=0; i<count; ++i) memcpy(pixels+i*3, palette+index[i]*3, 3);
		}
		else {
			byte mask = (1<<header.bitDepth) - 1;
			unsigned orow = header.width * 3;
			unsigned irow = (header.width * header.bitDepth + 7) >> 3;
			for(unsigned y=0; y<header.height; ++y) {
				byte* in = index + y * irow;
				byte* out = pixels + y * orow;
				byte* end = out + orow;
				while(out < end) {
					for(int s=8-header.bitDepth; s>=0; s-=header.bitDepth) {
						int k = (*in >> s) & mask;
						memcpy(out, palette+k*3, 3);
						out += 3;
					}
					++in;
				}
			}
		}
		delete [] palette;
		delete [] index;
	}

	if(format != Image::INVALID) return Image(format, header.width, header.height, pixels);
	delete [] pixels;
	return Image();
}


Image PNG::load(const char* file) {
	FILE* fp = fopen(file, "rb");
	if(!fp) return Image();
	
	unsigned length = 1024;
	byte* buffer = new byte[length];
	byte intBuffer[4];
	Image r = readPNGFile([fp, &buffer, &length, &intBuffer](unsigned len) {
		if(feof(fp)) {
			memset(buffer, 0, 4);
			return buffer;
		}
		if(len==4) {
			fread(intBuffer, 1, 4, fp);
			return intBuffer;
		}
		if(len > length) {
			delete [] buffer;
			buffer = new byte[len];
			length = len;
		}
		fread(buffer, 1, len, fp);
		return buffer;
	});

	fclose(fp);
	delete [] buffer;
	if(!r) printf("Unsupported or invalid png image %s\n", file);
	return r;
}

Image PNG::parse(const char* data, unsigned size) {
	const byte* stream = (const byte*)data;
	const byte* end = stream + size;
	return readPNGFile([&stream, end](unsigned len) {
		const byte* r = stream;
		stream += len;
		if(stream > end) return end-len; // error - end of stream
		return r;
	});
}

bool PNG::save(const base::Image& image, const char* file) {
	if(!image) return false;
	if(image.getFormat() > Image::RGBA8) return false;
	FILE* fp = fopen(file,"wb");
	if(!fp) return false;

	size_t length = 0;
	void* data = tdefl_write_image_to_png_file_in_memory(image.getData(), image.getWidth(), image.getHeight(), image.getChannels(), &length);
	fwrite(data, 1, length, fp);
	fclose(fp);
	mz_free(data);
	return true;
}

// TODO:
// - Write getBackground() function - return backgroundColour, or the first pixel (placeholder for BT stream)
// 		dds loader needs this functionality too


