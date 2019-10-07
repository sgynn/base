#include <base/dds.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace base;

#define DDS_CAPS        0x000001
#define DDS_HEIGHT      0x000002
#define DDS_WIDTH       0x000004
#define DDS_PITCH       0x000008
#define DDS_PIXELFORMAT 0x001000
#define DDS_MIPMAPCOUNT 0x020000
#define DDS_LINEARSIZE  0x080000
#define DDS_DEPTH       0x800000

#define DDS_ALPHA     0x0001
#define DDS_FOURCC    0x0004
#define DDS_RGB       0x0040
#define DDS_RGBA      0x0041
#define DDS_LUMINANCE 0x20000

#define DDS_COMPLEX   0x000008
#define DDS_TEXTURE   0x001000
#define DDS_MIPMAP    0x400000

#define DDS_CUBEMAP     0x200
#define DDS_CUBEMAP_X1  0x0400
#define DDS_CUBEMAP_X0  0x0800
#define DDS_CUBEMAP_Y1  0x1000
#define DDS_CUBEMAP_Y0  0x2000
#define DDS_CUBEMAP_Z1  0x4000
#define DDS_CUBEMAP_Z0  0x8000
#define DDS_CUBAMEP_ALL 0xfc00
#define DDS_VOLUME      0x200000

#define FOURCC_DXT1     0x31545844	// 'D' 'X' 'T' '1'
#define FOURCC_DXT3     0x33545844
#define FOURCC_DXT5     0x35545844
#define FOURCC_ATI1     0x31495441
#define FOURCC_ATI2     0x32495441

#define FOURCC_DX10     0x30315844

// DDS Header structures
namespace base {
typedef unsigned int DWORD;
typedef unsigned char BYTE;
struct DDSPixelFormat {
	DWORD size;
	DWORD flags;
	DWORD fourCC;
	DWORD bitCount;
	DWORD bitMask[4];
};
struct DDSHeader {
	DWORD size;
	DWORD flags;
	DWORD height;
	DWORD width;
	DWORD pitch;
	DWORD depth;
	DWORD mipmaps;
	DWORD reserved1[11];
	DDSPixelFormat format;
	DWORD caps1;
	DWORD caps2;
	DWORD reserved2[3];
};
inline DWORD calculateSize(DDS::DDSFormat f, int w, int h, int d) {
	switch(f) {
	case DDS::BC1:
	case DDS::BC4: return ((w+3)/4) * ((h+3)/4) * 8 * d;
	case DDS::BC2:
	case DDS::BC3:
	case DDS::BC5: return ((w+3)/4) * ((h+3)/4) * 16 * d;
	default:  return f * w * h * d;
	}
}


DDS::DDS(DDS&& o) : format(o.format), mode(o.mode), mipmaps(o.mipmaps), width(o.width), height(o.width), depth(o.depth), data(o.data) {
	o.data = 0;
	o.format = INVALID;
}
DDS& DDS::operator=(DDS&& o) {
	memcpy(this, &o, sizeof(DDS));
	o.data = 0;
	o.format = INVALID;
	return *this;
}
void DDS::clear() {
	if(!data) return;
	
	int surfaces = mode==CUBE? 6: 1;
	int total = surfaces * (mipmaps+1);
	for(int i=0; i<total; ++i) {
		delete [] data[i];
	}
	//delete [] data;
	format = INVALID;
	data = 0;
}



DDS DDS::load(const char* filename) {
	DDS dds;
	FILE* fp = fopen(filename, "rb");
	if(!fp) return dds;

	// Chech type header
	char magic[4];
	fread(magic, 1, 4, fp);
	if(strncmp(magic, "DDS ", 4)!=0) {
		fclose(fp);
		return dds;
	}

	// Read DDS header
	DDSHeader header;
	memset(&header, 0, sizeof(DDSHeader));
	fread(&header, sizeof(DDSHeader), 1, fp);
	// ToDo: Swap DWORD endian if MACOS
	
	// Get format info from header
	if(header.caps2 & DDS_CUBEMAP) dds.mode = CUBE;
	if(header.caps2 & DDS_VOLUME && header.depth>0) dds.mode = VOLUME;

	// Get compression
	if(header.format.flags == DDS_FOURCC) {
		switch(header.format.fourCC) {
		case FOURCC_DXT1: dds.format = BC1; break;
		case FOURCC_DXT3: dds.format = BC2; break;
		case FOURCC_DXT5: dds.format = BC3; break;
		case FOURCC_ATI1: dds.format = BC4; break;
		case FOURCC_ATI2: dds.format = BC5; break;
		default:
			// Note: FOURC_DX10 adds an additional header that allows texture arrays and floating point formats
			fclose(fp);
			return dds;
		}
	}
	// Non-compressed formats
	else if(header.format.flags == DDS_RGBA && header.format.bitCount == 32) dds.format = RGBA;
	else if(header.format.flags == DDS_RGB && header.format.bitCount == 32) dds.format = RGBA;
	else if(header.format.flags == DDS_RGB && header.format.bitCount == 24) dds.format = RGB;
	else if(header.format.bitCount == 8) dds.format = LUMINANCE;
	else {
		fclose(fp);
		return dds;
	}

	// Set image dimensions
	dds.width   = header.width;
	dds.height  = header.height;
	dds.depth   = header.depth;
	dds.mipmaps = header.mipmaps - 1;
	if(dds.depth<=0) dds.depth = 1;


	// Load all surfaces
	int surfaces = dds.mode==CUBE? 6: 1;
	dds.data = new BYTE*[surfaces * header.mipmaps];
	printf("DDS: %d surfaces, %d mipmaps\n", surfaces, dds.mipmaps);
	for(int n=0; n<surfaces; ++n) {
		int w = dds.width, h = dds.height, d = dds.depth;
		for(int i=0; i<=dds.mipmaps; ++i) {
			// Clamp size
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;

			// Read pixels
			DWORD size = calculateSize(dds.format, w, h, d);
			BYTE* pixels = new BYTE[size];
			fread(pixels, 1, size, fp);
			dds.data[ n + i * surfaces ] = pixels;

			// Next mipmap
			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	fclose(fp);
	return dds;
}

int DDS::save(const char* filename) {
	if(format==INVALID) return false;
	DDSHeader header;
	memset(&header, 0, sizeof(DDSHeader));
	header.size = sizeof(DDSHeader);
	header.flags = DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT;
	header.width = width;
	header.height = height;

	if(isCompressed()) {
		header.flags |= DDS_LINEARSIZE;
		header.pitch = calculateSize(format, width, height, depth);
	} else {
		header.flags |= DDS_PITCH;
		header.pitch = ((width * format * 8 + 31) & -32) >> 3; // 4 byte aligned width
	}

	if(mode == VOLUME) {
		header.flags |= DDS_DEPTH;
		header.depth = depth;
	}
	if(mipmaps > 0) {
		header.flags |= DDS_MIPMAPCOUNT;
		header.mipmaps = mipmaps + 1;
	}

	// Get pixel format
	header.format.size = sizeof(DDSPixelFormat);
	switch(format) {
	case INVALID:
		return false;
	case LUMINANCE:
		header.format.flags = DDS_LUMINANCE;
		header.format.bitMask[0] = 0xff;
		break;
	case LUMINANCE_ALPHA:
		header.format.flags = DDS_LUMINANCE | DDS_ALPHA;
		header.format.bitMask[0] = 0x00ff;
		header.format.bitMask[3] = 0xff00;
		break;
	case RGB:
		header.format.flags = DDS_RGB;
		header.format.bitMask[0] = 0xff0000;
		header.format.bitMask[1] = 0x00ff00;
		header.format.bitMask[2] = 0x0000ff;
		break;
	case RGBA:
		header.format.flags = DDS_RGBA;
		header.format.bitMask[0] = 0x00ff0000;
		header.format.bitMask[1] = 0x0000ff00;
		header.format.bitMask[2] = 0x000000ff;
		header.format.bitMask[3] = 0xff000000;
		break;
	case BC1:
		header.format.flags = DDS_FOURCC;
		header.format.fourCC = FOURCC_DXT1;
		break;
	case BC2:
		header.format.flags = DDS_FOURCC;
		header.format.fourCC = FOURCC_DXT3;
		break;
	case BC3:
		header.format.flags = DDS_FOURCC;
		header.format.fourCC = FOURCC_DXT5;
		break;
	case BC4:
		header.format.flags = DDS_FOURCC;
		header.format.fourCC = FOURCC_ATI1;
		break;
	case BC5:
		header.format.flags = DDS_FOURCC;
		header.format.fourCC = FOURCC_ATI2;
		break;
	}
	
	header.caps1 = DDS_TEXTURE;
	if(mode == CUBE) {
		header.caps1 |= DDS_COMPLEX;
		header.caps2 = DDS_VOLUME;
	}
	if(mipmaps > 0) {
		header.caps1 |= DDS_COMPLEX | DDS_MIPMAP;
	}

	// Open File
	FILE* fp = fopen(filename, "rb");
	if(!fp) return false;

	// Write header
	fwrite("DDS ", 1, 4, fp);
	fwrite(&header, 1, sizeof(DDSHeader), fp);

	// Write surfaces
	int surfaces = mode==CUBE? 6: 1;
	for(int n=0; n<surfaces; ++n) {
		int w = width, h = height, d = depth;
		for(int i=0; i<=mipmaps; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			int size = calculateSize(format, w,h,d);
			fwrite( data[n + i*surfaces], 1, size, fp);
			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	fclose(fp);
	return true;
}


void DDS::flip() {
	// Flip image
	int surfaces = mode==CUBE? 6: 1;
	for(int n=0; n<surfaces; ++n) {
		int w = width, h = height, d = depth;
		for(int i=0; i<=mipmaps; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			flipTexture( data[ n + i * surfaces ], w, h, d);
			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	// Cubemaps need y axis surfaces swapping (surfaces 2 and 3)
	if(mode==CUBE) {
		for(int i=0; i<=mipmaps; ++i) {
			BYTE* tmp = data[2 + i*surfaces];
			data[2+i*surfaces] = data[3+i*surfaces];
			data[3+i*surfaces] = tmp;
		}
	}
}

void DDS::decompress() {
	if(!isCompressed()) return;
	static DDSFormat targets[] = { RGB, RGBA, RGBA, LUMINANCE, LUMINANCE_ALPHA };
	int bpp = (int)targets[ format - BC1 ];
	int surfaces = mode==CUBE? 6: 1;
	for(int n=0; n<surfaces; ++n) {
		int w = width, h = height, d = depth;
		for(int i=0; i<=mipmaps; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			
			BYTE* out = new BYTE[w*h*d*bpp];
			BYTE* old = data[ n + i * surfaces];
			decompress(old, w, h, out);
			data[ n + i * surfaces] = out;
			delete [] old;

			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	format = targets[ format - BC1 ];
}


// --------------------------------------------------------------------------- //


inline void swapMemory(void* a, void* b, void* tmp, size_t size) {
	memcpy(tmp, a, size);
	memcpy(a, b, size);
	memcpy(b, tmp, size);
}

void DDS::flipTexture(BYTE* data, int w, int h, int d) {
	if(!isCompressed()) {
		int line = w * format;;
		BYTE* tmp = new BYTE[line];
		for(int n=0; n<d; ++n) {
			BYTE* p = data + n * line * h;
			for(int i=0; i<h/2; ++i) {
				swapMemory(p+i*line, p+(h-i-1)*line, tmp, line);
			}
		}
		delete [] tmp;
	}
	// Flipping compressed images is more complicated
	else {
		int bx = w/4;
		int by = h/4;
		int bs = format==BC1 || format==BC4? 8: 16;
		int line = bx * bs;

		BYTE* tmp = new BYTE[bs * bx];

		// Colour block (BC1~BC3) { short c0, c1; byte row[4]; }	// lookup 2 bits per pixel
		// Alpha block  (BC3,4,5) { byte a0, a1; byte row[6];  }	// lookup 3bits per pixel
		// BC2 alpha block (BC2)  { byte row[4];               }	// absolute value

		// move blocks
		for(int i=0; i<by/2; ++i) {
			swapMemory(data + i*line, data + (by-i-1)*line, tmp, line);
		}
		delete [] tmp;
		
		// Flip blocks
		BYTE tc;
		switch(format) {
		case BC1:
			for(int i=0; i<bx*by; ++i) {
				BYTE* block = data + i * bs;
				swapMemory(block+4, block+7, &tc, 1);
				swapMemory(block+5, block+6, &tc, 1);
			}
			break;
		case BC2:
			for(int i=0; i<bx*by; ++i) {
				BYTE* block = data + i * bs;
				swapMemory(block+0, block+3, &tc, 1);
				swapMemory(block+1, block+2, &tc, 1);
				swapMemory(block+12, block+15, &tc, 1);
				swapMemory(block+13, block+14, &tc, 1);
			}
			break;
		case BC3:
			for(int i=0; i<bx*by; ++i) {
				BYTE* block = data + i * bs;
				swapMemory(block+12, block+15, &tc, 1);
				swapMemory(block+13, block+14, &tc, 1);
				// Flip DXT5 alpha - this method may have endian issues. if so, just shift by 4;
				unsigned long long a, b;
				memcpy(&a, block+2, 6);
				a >>= 4;
				b = ((a&07000)>>9) | ((a&0700)>>3) | ((a&070)<<3) | ((a&07)<<9);
				b <<= 4;
				memcpy(block+2, &b, 6);
			}
			break;
		case BC4:
		case BC5:
		default: break; // invalid
		}
	}
}

// --------------------------------------------------------------------------- //

int DDS::decodeR5G6B5(const BYTE* p) {
	BYTE r = (p[1]>>3) * 255 / 31;
	BYTE g = ((p[1]<<3 | p[0]>>5) & 0x3f) * 255 / 63;
	BYTE b = (p[0]&0x1f) * 255 / 31;
	return r<<16 | g<<8 | b;
}
void DDS::createTable(const BYTE* a, const BYTE* b, BYTE* o) {
	int ca = decodeR5G6B5(a);
	int cb = decodeR5G6B5(b);
	o[0] = ca>>16;
	o[1] = (ca>>8) & 0xff;
	o[2] = ca & 0xff;

	o[3] = cb>>16;
	o[4] = (cb>>8) & 0xff;
	o[5] = cb & 0xff;

	o[6] = (o[0] * 2 + o[3]) / 3;
	o[7] = (o[1] * 2 + o[4]) / 3;
	o[8] = (o[2] * 2 + o[5]) / 3;

	o[9]  = (o[0] + o[3] * 2) / 3;
	o[10] = (o[1] + o[4] * 2) / 3;
	o[11] = (o[2] + o[5] * 2) / 3;
}
void DDS::createBC4Table(BYTE a, BYTE b, BYTE* o) {
	o[0] = a;
	o[1] = b;
	if(a>b) {
		o[2] = (a*6 + b*1) / 7;
		o[3] = (a*5 + b*2) / 7;
		o[4] = (a*4 + b*3) / 7;
		o[5] = (a*3 + b*4) / 7;
		o[6] = (a*2 + b*5) / 7;
		o[7] = (a*1 + b*6) / 7;
	} else {
		o[2] = (a*4 + b*1) / 5;
		o[3] = (a*3 + b*2) / 5;
		o[4] = (a*2 + b*3) / 5;
		o[5] = (a*1 + b*4) / 5;
		o[6] = 0;
		o[7] = 255;
	}
}

void DDS::decompress(const BYTE* data, int w, int h, BYTE* out) const {
	int bx = w/4;
	int by = h/4;
	int bs = format==BC1? 8: 16;
	int line = bx * bs;
	BYTE c[12];

	// Colour block
	if(format <= BC3) {
		int bpp = format==BC1? 3: 4;
		const BYTE* cdata = format==BC1? data: data+8;
		BYTE* cout = out + (format==BC1? 0: 1);
		for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
			const BYTE* block = cdata + x * bs + y*line;
			// decode R5G6b5 colour
			createTable(block, block+2, c);
			// Decode pixels
			for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
				int p = (block[j+4]>>(i*2)) & 3;
				BYTE* px = cout + (x*4+i + (y*4+j) * w) * bpp;
				memcpy( px, c+p*3, 3 );
			}
		}
	}

	switch(format) {
	case BC2: // BC2 alpha
		for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
			const BYTE* block = data + x * bs + y*line;
			for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
				int a = (block[j+4]>>(i*2)) & 3;
				BYTE* px = out + (x*4+i + (y*4+j) * w) * 4; // bpp=4
				px[0]  = a * 85;
			}
		}
		break;
	case BC3:	// BC3 Alpla
		decompressBC4Channel(data, bx, by, 16, out, 4);
		break;
	case BC4:	// Single BC4 block
		decompressBC4Channel(data, bx, by, 8, out, 1);
		break;
	case BC5:	// Dual BC4 blocks
		decompressBC4Channel(data, bx, by, 16, out, 2);
		decompressBC4Channel(data+8, bx, by, 16, out+1, 2);
		break;
	default: break;
	}
}

void DDS::decompressBC4Channel(const BYTE* data, int bx, int by, int stride, BYTE* out, int bpp) {
	BYTE c[8];
	int w = bx * 4;
	int line = bx * stride;
	unsigned long long b;
	for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
		const BYTE* block = data + x * stride + y*line;
		createBC4Table(block[0], block[1], c);
		memcpy(&b, block, 8);
		for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
			int k = i + j*4;
			int v = (b >> (48 - k*3)) & 7;
			BYTE* px = out + (x*4+i + (y*4+j) * w) * bpp;
			px[0] = c[v];
		}
	}
}



}

