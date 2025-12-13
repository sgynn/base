// Tiff file streaming height data

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <base/tiff.h>

using namespace base;

enum Tiff_FTag {
	NEW_SUB_FILE_TYPE          = 254,	// 4,1,0
	WIDTH                      = 256,	// 3,1,16385
	HEIGHT                     = 257,	// 3,1,16385
	BITS_PER_SAMPLE            = 258,	// 3,1,16
	COMPRESSION                = 259,	// 3,1,1
	PHOTOMETRIC_INTERPRETATION = 262,	// 3,1,1
	DESCRIPTION                = 270,	// 2,length,address
	STRIP_OFFSETS              = 273,	// 4,1,?		* image data *
	ORIENTATION                = 274,   // 3,1,1
	SAMPLES_PER_PIXEL          = 277,	// 3,1,1
	ROWS_PER_STRIP             = 278,	// 3,1,16385
	STRIP_BYTE_COUNTS          = 279,	// 4,1,536936540
	X_RESOLUTION               = 282,	// 5,1,?
	Y_RESOLUTION               = 283,	// 5,1,?
	PLANAR_CONFIGURATOIN       = 284,
	RESOLUTION_UNIT            = 296,	// 3,1,2
	COLOUR_RESPONSE_CURVES     = 301,
	SOFTWARE                   = 305,	// 2,28,?
	DATE_TIME                  = 306,	// 2,20,? [YYYY:MM:DD HH:MM:SS]
	COLOUR_MAP                 = 320,
	GRAY_RESPONSE_CURVE        = 291,	
	GRAY_RESPONSE_UNIT         = 290,
	PREDICTOR                  = 317,
	ARTIST                     = 315,
	SAMPLEFORMAT               = 339,	// 3,1,3
};

enum Tiff_FType {
	BYTE     = 1,
	STRING   = 2,
	WORD     = 3,
	DWORD    = 4,
	RATIONAL = 5
};

struct Tiff_IFD {
	unsigned short tag;
	unsigned short type;
	unsigned int   length;
	unsigned int   offset;
};
inline void setTiffDesc(Tiff_IFD& d, unsigned short tag, unsigned short type, unsigned int length, unsigned int offset) {
	d.tag=tag; d.type=type; d.length=length; d.offset=offset;
}




// Implementation

typedef unsigned int dword;
typedef unsigned short word;


TiffStream* TiffStream::openStream( const char* file, Mode mode) {
	const char* modes[3] = { "rb", "wb", "r+b" };
	FILE* fp = fopen(file, modes[mode-1]);
	if(!fp) return 0;


	// Read header
	char header[4];
	fread(header, 4, 1, fp);
	if(header[0]!='I' || header[1]!='I' || header[2]!=42 || header[3]!=0) {
		printf("Invalid TIFF header\n");
		fclose(fp);
		fp = 0;
		return 0;
	}

	TiffStream* img = new TiffStream();
	img->m_stream = fp;
	dword stripOffsets = 0;
	dword stripCount = 0;

	printf("Loading %s\n", file);

	// Read field descriptors
	word count;
	dword offset;
	fread(&offset, 4, 1, fp);
	Tiff_IFD desc;
	while(offset>0) {
		fseek(fp, offset, SEEK_SET);
		fread(&count, 2, 1, fp);
		for(word i=0; i<count; ++i) {
			fread(&desc, 12, 1, fp);

			//printf("Tag: %d %d %d %d\n", desc.tag, desc.type, desc.length, desc.offset);
			// Handle descriptor
			switch(desc.tag) {
			case WIDTH:           img->m_width = desc.offset; break;
			case HEIGHT:          img->m_height = desc.offset; break;
			case BITS_PER_SAMPLE: img->m_bitsPerSample[0] = desc.offset&0xffff; break;
			case COMPRESSION: if(desc.offset!=1) { printf("Error: Image compressed %d\n", desc.offset); return 0; } break;
			case PHOTOMETRIC_INTERPRETATION: break;
			case STRIP_OFFSETS:
				stripCount = desc.length;
				stripOffsets = desc.offset; // If length>1, this is a pointer
				break;
			case SAMPLES_PER_PIXEL: img->m_samplesPerPixel = desc.offset; break;
			case ROWS_PER_STRIP:    img->m_rowsPerStrip = desc.offset; break;
			case SAMPLEFORMAT:      img->m_sampleFormat = desc.offset; break;
			default: break;
			}
		}
		fread(&offset, 4, 1, fp);
	}

	// Fix bitsPerSample (assume it is a word)
	if(img->m_samplesPerPixel>1) {
		fseek(fp, img->m_bitsPerSample[0], SEEK_SET);
		word b[4] = {0,0,0,0};
		fread(b, img->m_samplesPerPixel, 2, fp);
		for(unsigned i=0; i<img->m_samplesPerPixel; ++i) img->m_bitsPerSample[i] = b[i];

		// Sample Format (1=uint, 2=int, 3=float, 4=void) may or may not be per sample
		// Technically tiff allows this to be different per sample
		if(img->m_sampleFormat > 8) {
			fseek(fp, img->m_sampleFormat, SEEK_SET);
			fread(b, img->m_samplesPerPixel, 2, fp);
			img->m_sampleFormat = b[0];
		}
	}

	img->m_bitsPerPixel = 0;
	for(unsigned i=0; i<img->m_samplesPerPixel; ++i) img->m_bitsPerPixel += img->m_bitsPerSample[i];


	// Update strip offsets
	if(stripCount>1) {
		img->m_stripCount = stripCount;
		img->m_stripOffsets = new unsigned[ stripCount ];
		fseek(fp, stripOffsets, SEEK_SET);
		fread(img->m_stripOffsets, 4, stripCount, fp);
	}
	else if(stripCount==1) {
		img->m_stripCount = 1;
		img->m_stripOffsets = new unsigned[1];
		img->m_stripOffsets[0] = stripOffsets;
	}


	return img;

}

TiffStream* TiffStream::createStream(const char* file, int w, int h, Image::Format format, Mode mode, const void* data, size_t len) {
	if(Image::getPixelType(format) == Image::UNKNOWN) return nullptr;

	// Create a tiff file
	const char* modes[] = { 0, "rb", "wb", "w+b" };
	FILE* fp = fopen(file, modes[mode]);
	if(!fp) return nullptr;

	const char* software = "Heightmap Editor";
	char dateTime[21] = "YYYY:MM:DD HH::MM:SS";
	time_t t = time(0);
	strftime(dateTime, 21, "%Y:%m:%d %H:%M:%S", localtime(&t));

	constexpr int bytesPerChannelLookup[] = { 0, 1, 2, 2, 4 };
	int bytesPerChannel = bytesPerChannelLookup[Image::getPixelType(format)];
	int channels = Image::getChannels(format);

	// Write header
	fwrite("II*", 1, 4, fp);

	// Create stream
	TiffStream* img = new TiffStream();
	img->m_stream = fp;
	img->m_width = w;
	img->m_height = h;
	img->m_samplesPerPixel = channels;
	img->m_bitsPerPixel = channels * bytesPerChannel * 8;
	img->m_sampleFormat = Image::getPixelType(format) > Image::WORD? 3: 1; // float or uint
	img->m_rowsPerStrip = h;
	img->m_stripCount = 1;
	img->m_stripOffsets = new unsigned[1];
	for(int i=0; i<channels; ++i) img->m_bitsPerSample[i] = bytesPerChannel * 8;


	// Create field descriptors
	dword offset = 8;
	fwrite(&offset, 1, 4, fp); // Offset of descriptors
	Tiff_IFD desc[20];
	setTiffDesc(desc[0],  NEW_SUB_FILE_TYPE,          DWORD, 1, 0);
	setTiffDesc(desc[1],  WIDTH,                      WORD,  1, w);	// can be DWORD
	setTiffDesc(desc[2],  HEIGHT,                     WORD,  1, h);
	setTiffDesc(desc[3],  BITS_PER_SAMPLE,            WORD,  1, img->m_bitsPerSample[0]);
	setTiffDesc(desc[4],  COMPRESSION,                WORD,  1, 1);				// No Compression
	setTiffDesc(desc[5],  PHOTOMETRIC_INTERPRETATION, WORD,  1, 1);
	setTiffDesc(desc[6],  STRIP_OFFSETS,              DWORD, 1, 0); 			// Image data offset - Will be static
	setTiffDesc(desc[7],  ORIENTATION,                WORD,  1, 1);
	setTiffDesc(desc[8],  SAMPLES_PER_PIXEL,          WORD,  1, img->m_samplesPerPixel);
	setTiffDesc(desc[9],  ROWS_PER_STRIP,             WORD,  1, h); 			// All rows in one strip
	setTiffDesc(desc[10], STRIP_BYTE_COUNTS,          DWORD, 1, w*h*channels*bytesPerChannel);
	setTiffDesc(desc[11], X_RESOLUTION,               RATIONAL, 1, 0);		// Umm, meh
	setTiffDesc(desc[12], Y_RESOLUTION,               RATIONAL, 1, 0);
	setTiffDesc(desc[13], RESOLUTION_UNIT,            WORD,   1, 2);
	setTiffDesc(desc[14], SOFTWARE,                   STRING, strlen(software)+1, 0); // calulate offset
	setTiffDesc(desc[15], DATE_TIME,                  STRING, 20, 0);
	setTiffDesc(desc[16], SAMPLEFORMAT,               WORD, 1, img->m_sampleFormat);
	word count = 17;

	offset += sizeof(word) + count * sizeof(Tiff_IFD) + sizeof(dword);
	desc[11].offset = offset; offset+=8;
	desc[12].offset = offset; offset+=8;
	if(channels > 1) {
		desc[3].offset = offset; offset += 2 * channels;
		desc[16].offset = offset; offset += 2 * channels;
	}
	desc[14].offset = offset; offset+=desc[14].length;
	desc[15].offset = offset; offset+=desc[15].length;
	desc[ 6].offset = offset; offset+=desc[10].offset; // Data
	img->m_stripOffsets[0] = desc[6].offset;

	// Write descriptors
	dword next = 0;
	fwrite(&count, sizeof(word), 1, fp);
	fwrite(desc, sizeof(Tiff_IFD), count, fp);
	fwrite(&next, sizeof(dword), 1, fp); // RESOLUTION
	if(channels > 1) {
		fwrite(img->m_bitsPerSample, 2, channels, fp);
		word type = img->m_sampleFormat;
		for(int i=0; i<channels; ++i) fwrite(&type, 2, 1, fp);
	}

	// Write data
	dword resolution[2] = { 720000, 10000 }; // RATIONAL: numerator, denominator
	fwrite(resolution, 4, 2, fp);
	fwrite(resolution, 4, 2, fp);
	fwrite(software, 1, desc[14].length, fp);
	fwrite(dateTime, 1, desc[15].length, fp);

	// Initialise data
	size_t bytes = desc[10].offset;
	if(data && (len==0 || len==bytes)) fwrite(data, 1, bytes, fp);
	else {
		// Set all pixels to 0
		static const int bs = 6000;
		char buffer[bs];
		memset(buffer, 0, bs);
		while(bytes > bs) bytes -= fwrite(buffer, 1, bs, fp);
		if(bytes>0) fwrite(buffer, 1, bytes, fp);
	}
		
	return img;
}

inline size_t TiffStream::getAddress(int x, int y) const {
	unsigned strip = y / m_rowsPerStrip;
	unsigned index = x + (y-strip*m_rowsPerStrip) * m_width;
	unsigned bpp = m_bitsPerPixel / 8;
	return m_stripOffsets[strip] + bpp*index;
}


size_t TiffStream::getPixel(int x, int y, void* data) const {
	size_t addr = getAddress(x, y);
	fseek(m_stream, addr, SEEK_SET);
	#ifdef LINUX
	if(feof(m_stream)) asm("int $3");
	#endif
	return fread(data, m_bitsPerPixel/8, 1, m_stream);
}

size_t TiffStream::setPixel(int x, int y, void* data) {
	size_t addr = getAddress(x, y);
	fseek(m_stream, addr, SEEK_SET);
	return fwrite(data, m_bitsPerPixel, 1, m_stream);
}


size_t TiffStream::readBlock(int x, int y, int width, int height, void* data) const {
	if(x<=-width || y<=-height || x>=(int)m_width || y>=(int)m_height) return 0;	// fully outside image
	// assume data is correctly allocated
	unsigned bytes = m_bitsPerPixel / 8;
	size_t offset = x<0? -x: 0;
	size_t len = x+width > (int)m_width? m_width-x-offset: width-offset;
	size_t count = 0;
	int y0 = y<0? -y: 0;
	int y1 = y+height > (int)m_height? m_height-y: height;
	for(int i=y0; i<y1; ++i) {
		size_t addr = getAddress(x+offset, y+i);
		fseek(m_stream, addr, SEEK_SET);
		count += fread((char*)data + i * bytes * width + offset*bytes, bytes, len, m_stream);
	}
	return count;
}
size_t TiffStream::writeBlock(int x, int y, int width, int height, const void* data) {
	if(x<=-width || y<=-height || x>=(int)m_width || y>=(int)m_height) return 0;	// fully outside image
	unsigned bytes = m_bitsPerPixel / 8;
	size_t offset = x<0? -x: 0;
	size_t len = x+width > (int)m_width? m_width-x-offset: width-offset;
	size_t count = 0;
	int y0 = y<0? -y: 0;
	int y1 = y+height > (int)m_height? m_height-y: height;
	for(int i=y0; i<y1; ++i) {
		size_t addr = getAddress(x+offset, y+i);
		fseek(m_stream, addr, SEEK_SET);
		count += fwrite((const char*)data + i * bytes * width + offset * bytes, bytes, len, m_stream);
	}
	return count;
}


TiffStream::~TiffStream() {
	if(m_stream) fclose(m_stream);
}

// =================================================================================== //

Image Tiff::load(const char* file) {
	TiffStream* stream = TiffStream::openStream(file);
	if(stream) {
		int width = stream->width();
		int height = stream->height();
		Image::Format format = Image::INVALID;

		unsigned ch = stream->channels();
		unsigned bpp = stream->bpp();
		if(stream->sampleFormat() == 1) { // UINT
			if(bpp==16 && ch==1) format = Image::R16;
			else if(bpp==8*ch) format = (Image::Format)ch;
		}
		else if(stream->sampleFormat() == 3) { // Float
			if(bpp == 16 && ch == 1) format = Image::R16F;
			else if(bpp == 32 && ch == 1) format = Image::R32F;
		}

		if(format == Image::INVALID) {
			const char* types[] = { "Invalid", "uint", "int", "float", "void", "complex int", "complex float" };
			printf("Unhandled Tiff image format [%d bits, %d channels ", bpp, ch);
			if(ch>1) for(unsigned i=0;i<ch;++i) printf("%d ", stream->m_bitsPerSample[i]);
			printf("%s]\n", types[stream->sampleFormat()]);
			return Image();
		}


		int s = width * height * stream->bpp() / 8;
		Image::byte* data = new Image::byte[s];
		stream->readBlock(0, 0, width, height, data);
		delete stream;
		return Image(format, width, height, data);
	}
	return Image();
}

bool Tiff::save(const Image& image, const char* file) {
	if(!image) return false;
	TiffStream* stream = TiffStream::createStream(file, image.getWidth(), image.getHeight(), image.getFormat(),
					TiffStream::WRITE, image.getData(), image.getWidth()*image.getHeight()*image.getBytesPerPixel());
	delete stream;
	return stream;
}

