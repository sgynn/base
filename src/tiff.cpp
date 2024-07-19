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
			case BITS_PER_SAMPLE: img->m_bitsPerSample = desc.offset&0xffff; break;
			case COMPRESSION: if(desc.offset!=1) { printf("Error: Image compressed\n"); return 0; }
			case PHOTOMETRIC_INTERPRETATION: break;
			case STRIP_OFFSETS:
				stripCount = desc.length;
				stripOffsets = desc.offset; // If length>1, this is a pointer
				break;
			case SAMPLES_PER_PIXEL: img->m_samplesPerPixel = desc.offset; break;
			case ROWS_PER_STRIP:    img->m_rowsPerStrip = desc.offset; break;
			default: break;
			}
		}
		fread(&offset, 4, 1, fp);
	}

	// Fix bitsPerSample (assume it is a word)
	if(img->m_samplesPerPixel>1) {
		fseek(fp, img->m_bitsPerSample, SEEK_SET);
		word b[4] = {0,0,0,0};
		fread(b, img->m_samplesPerPixel, 2, fp);
		img->m_bitsPerSample = b[0];
		//printf("bps: %d %d %d %d\n", b[0], b[1], b[2], b[3]);
	}

	// Update strip offsets
	if(stripCount>1) {
		img->m_stripCount = stripCount;
		img->m_stripOffsets = new uint[ stripCount ];
		fseek(fp, stripOffsets, SEEK_SET);
		fread(img->m_stripOffsets, 4, stripCount, fp);
	} else if(stripCount==1) {
		img->m_stripCount = 1;
		img->m_stripOffsets = new uint[1];
		img->m_stripOffsets[0] = stripOffsets;
	}


	return img;

}

TiffStream* TiffStream::createStream(const char* file, int w, int h, int ch, int bpc, Mode mode, const void* data, size_t len) {
	// Create a tiff file
	const char* modes[] = { 0, "rb", "wb", "w+b" };
	FILE* fp = fopen(file, modes[mode]);
	if(!fp) return 0;

	const char* software = "Heightmap Editor";
	char dateTime[21] = "YYYY:MM:DD HH::MM:SS";
	time_t t = time(0);
	strftime(dateTime, 21, "%Y:%m:%d %H:%M:%S", localtime(&t));


	// Write header
	fwrite("II*", 1, 4, fp);

	// Create strea1
	TiffStream* img = new TiffStream();
	img->m_stream = fp;
	img->m_width = w;
	img->m_height = h;
	img->m_samplesPerPixel = ch;
	img->m_bitsPerSample = bpc;
	img->m_rowsPerStrip = h;
	img->m_stripCount = 1;
	img->m_stripOffsets = new uint[1];


	// Create field descriptors
	dword offset = 8;
	fwrite(&offset, 1, 4, fp); // Offset of descriptors
	Tiff_IFD desc[20];
	setTiffDesc(desc[0],  NEW_SUB_FILE_TYPE,          DWORD, 1, 0);
	setTiffDesc(desc[1],  WIDTH,                      WORD,  1, w);	// can be DWORD
	setTiffDesc(desc[2],  HEIGHT,                     WORD,  1, h);
	setTiffDesc(desc[3],  BITS_PER_SAMPLE,            WORD,  1, bpc);
	setTiffDesc(desc[4],  COMPRESSION,                WORD,  1, 1);				// No Compression
	setTiffDesc(desc[5],  PHOTOMETRIC_INTERPRETATION, WORD,  1, 1);
	setTiffDesc(desc[6],  STRIP_OFFSETS,              DWORD, 1, 0); 			// Image data offset - Will be static
	setTiffDesc(desc[7],  ORIENTATION,                WORD,  1, 1);
	setTiffDesc(desc[8],  SAMPLES_PER_PIXEL,          WORD,  1, ch);
	setTiffDesc(desc[9],  ROWS_PER_STRIP,             WORD,  1, h); 			// All rows in one strip
	setTiffDesc(desc[10], STRIP_BYTE_COUNTS,          DWORD, 1, w*h*ch*(bpc/8));
	setTiffDesc(desc[11], X_RESOLUTION,               RATIONAL, 1, 0);		// Umm, meh
	setTiffDesc(desc[12], Y_RESOLUTION,               RATIONAL, 1, 0);
	setTiffDesc(desc[13], RESOLUTION_UNIT,            WORD,   1, 2);
	setTiffDesc(desc[14], SOFTWARE,                   STRING, strlen(software), 0); // calulate offset
	setTiffDesc(desc[15], DATE_TIME,                  STRING, 20, 0);
	word count = 16;

	offset += sizeof(word) + count * sizeof(Tiff_IFD) + sizeof(dword);
	desc[11].offset = offset; offset+=8;
	desc[12].offset = offset; offset+=8;
	desc[14].offset = offset; offset+=desc[14].length;
	desc[15].offset = offset; offset+=desc[15].length;
	desc[ 6].offset = offset; offset+=desc[10].offset; // Data
	img->m_stripOffsets[0] = desc[6].offset;

	// Write descriptors
	dword next = 0;
	fwrite(&count, sizeof(word), 1, fp);
	fwrite(desc, sizeof(Tiff_IFD), count, fp);
	fwrite(&next, sizeof(dword), 1, fp);

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
		for(dword i=0; bytes>bs; i+=bs) bytes -= fwrite(buffer, 1, bs, fp);
		if(bytes>0) fwrite(buffer, 1, bytes, fp);
	}
		
	return img;
}

inline size_t TiffStream::getAddress(int x, int y) const {
	uint strip = y / m_rowsPerStrip;
	uint index = x + (y-strip*m_rowsPerStrip) * m_width;
	uint bpp = m_bitsPerSample/8 * m_samplesPerPixel;
	return m_stripOffsets[strip] + bpp*index;
}


size_t TiffStream::getPixel(int x, int y, void* data) const {
	size_t addr = getAddress(x, y);
	fseek(m_stream, addr, SEEK_SET);
	#ifdef LINUX
	if(feof(m_stream)) asm("int $3");
	#endif
	return fread(data, m_bitsPerSample/8, m_samplesPerPixel, m_stream);
}

size_t TiffStream::setPixel(int x, int y, void* data) {
	size_t addr = getAddress(x, y);
	fseek(m_stream, addr, SEEK_SET);
	return fwrite(data, m_bitsPerSample/8, m_samplesPerPixel, m_stream);
}


size_t TiffStream::readBlock(int x, int y, int width, int height, void* data) const {
	if(x<=-width || y<=-height || x>=(int)m_width || y>=(int)m_height) return 0;	// fully outside image
	// assume data is correctly allocated
	uint bytes = m_bitsPerSample/8 * m_samplesPerPixel;
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
	uint bytes = m_bitsPerSample/8 * m_samplesPerPixel;
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
		if(stream->channels() == 1 && stream->bpp() == 16) format = Image::R16;
		else if(stream->bpp() / stream->channels() == 8) format = (Image::Format)stream->channels();
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
	TiffStream* stream = TiffStream::createStream(file, image.getWidth(), image.getHeight(),
					image.getChannels(), image.getBytesPerPixel() / image.getChannels(),
					TiffStream::WRITE, image.getData(), image.getWidth()*image.getHeight()*image.getBytesPerPixel());
	delete stream;
	return stream;
}

