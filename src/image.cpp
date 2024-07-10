#include <base/image.h>
#include <cstring>
#include <cmath>

using namespace base;

Image::Image(Format f, int w, int h, byte* data) : Image(FLAT, f, w, h, 1, 1, data? new byte*[1]: nullptr) {
	if(data) m_data[0] = data;
}

Image::Image(Mode m, Format f, int w, int h, int d, int mips, byte** data)
	: m_mode(m)
	, m_format(f)
	, m_width(w)
	, m_height(h)
	, m_depth(d)
	, m_mips(mips)
	, m_data(data)
{
	if(!m_data) {
		// Create blank image
		unsigned ss = m_width * m_height * getBytesPerPixel();
		m_data = new byte*[m_mips];
		for(int i=0; i<m_mips; ++i) {
			m_data[i] = new byte[ss];
			memset(m_data[i], 0, ss);
			ss >>= 2;
		}
	}
}

Image::Image(Image&& o) noexcept
	: m_mode(o.m_mode)
	, m_format(o.m_format)
	, m_width(o.m_width)
	, m_height(o.m_height)
	, m_depth(o.m_depth)
	, m_mips(o.m_mips)
	, m_data(o.m_data)
{
	o.m_data = nullptr;
}


Image::~Image() {
	if(!m_data) return;
	int faces = m_mode==CUBE? m_mips*6: m_mips;
	for(byte i=0; i<faces; ++i) delete [] m_data[i];
	delete [] m_data;
}

Image& Image::operator=(Image&& o) noexcept {
	m_mode = o.m_mode;
	m_format = o.m_format;
	m_width = o.m_width;
	m_height = o.m_height;
	m_depth = o.m_depth;
	m_mips = o.m_mips;
	m_data = o.m_data;
	o.m_data = nullptr;
	return *this;
}

Image::byte* Image::stealData(int face, int mip) {
	if(face<getFaces() && mip<getMips()) {
		int k = face + mip * getFaces();
		byte* d = m_data[k];
		m_data[k] = nullptr;
		return d;
	}
	return nullptr;
}

// ===================================================================== //

Image::PixelType Image::getPixelType(Format f) {
	switch(f) {
	case R8: case RG8: case RGB8: case RGBA8: return BYTE;
	case R16: return WORD;
	case R16F: return HALFFLOAT;
	case R32F: return FLOAT;
	default: return UNKNOWN; // invalid or compressed types
	}
}

// ===================================================================== //



void Image::flip() {
	// Flip image
	int surfaces = getFaces();
	for(int n=0; n<surfaces; ++n) {
		int w = m_width, h = m_height, d = m_depth;
		for(int i=0; i<m_mips; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			flipSurface( m_data[ n + i * surfaces ], w, h, d);
			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	// Cubemaps need y axis surfaces swapping (surfaces 2 and 3)
	if(m_mode == CUBE) {
		for(int i=0; i<m_mips; ++i) {
			byte* tmp = m_data[2 + i*surfaces];
			m_data[2+i*surfaces] = m_data[3+i*surfaces];
			m_data[3+i*surfaces] = tmp;
		}
	}
}

void Image::flipSurface(byte* data, int w, int h, int d) {
	auto swapMemory = [](void* a, void* b, void* tmp, size_t size) {
		memcpy(tmp, a, size);
		memcpy(a, b, size);
		memcpy(b, tmp, size);
	};

	if(!isCompressed()) {
		int line = w * getBytesPerPixel();
		byte* tmp = new Image::byte[line];
		for(int n=0; n<d; ++n) {
			byte* p = data + n * line * h;
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
		int bs = m_format==BC1 || m_format==BC4? 8: 16;
		int line = bx * bs;

		byte* tmp = new byte[bs * bx];

		// Colour block (BC1~BC3) { short c0, c1; byte row[4]; }	// lookup 2 bits per pixel
		// Alpha block  (BC3,4,5) { byte a0, a1; byte row[6];  }	// lookup 3bits per pixel
		// BC2 alpha block (BC2)  { byte row[4];               }	// absolute value

		// move blocks
		for(int i=0; i<by/2; ++i) {
			swapMemory(data + i*line, data + (by-i-1)*line, tmp, line);
		}
		delete [] tmp;
		
		// Flip blocks
		byte tc;
		switch(m_format) {
		case BC1:
			for(int i=0; i<bx*by; ++i) {
				byte* block = data + i * bs;
				swapMemory(block+4, block+7, &tc, 1);
				swapMemory(block+5, block+6, &tc, 1);
			}
			break;
		case BC2:
			for(int i=0; i<bx*by; ++i) {
				byte* block = data + i * bs;
				swapMemory(block+0, block+3, &tc, 1);
				swapMemory(block+1, block+2, &tc, 1);
				swapMemory(block+12, block+15, &tc, 1);
				swapMemory(block+13, block+14, &tc, 1);
			}
			break;
		case BC3:
			for(int i=0; i<bx*by; ++i) {
				byte* block = data + i * bs;
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



// ===================================================================== //

/*
template<typename S, int SN typename T, int TN>
void convertArray(const S* src, const T* dest, int count) {
	constexpr int copy = SN<TN? SN: TN;
	while(count-->0) {
		for(int i=0; i<copy; ++i) dest[i] = src[i];
		src += SN;
		dest += TN;
	}
}
*/

using byte = Image::byte;

Image Image::convert(Format to, const char* swizzle) const {
	if(isCompressed(to)) return Image(); // Not implemented
	int surfaces = getFaces();
	byte** data = new byte*[surfaces * m_mips];
	for(int n=0; n<surfaces; ++n) {
		int w = m_width, h = m_height, d = m_depth;
		for(int i=0; i<m_mips; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			
			int k = n * i * surfaces;
			data[k] = convertSurface(m_data[k], w, h, d, to, swizzle);

			w = w>>1;
			h = h>>1;
			d = d>>1;

			if(!data[k]) { // Could not convert formats
				delete [] data;
				return Image();
			}
		}
	}
	return Image(m_mode, to, m_width, m_height, m_depth, m_mips, data);
}

int getPixelOffset(Image::Format format, char swizzle) {
	switch(swizzle) {
	case 'r':
		switch(format) {
		case Image::R8: case Image::RG8: case Image::RGB8: case Image::RGBA8: return 0;
		default: return -1;
		}
		break;
	case 'g':
		switch(format) {
		case Image::RG8: case Image::RGB8: case Image::RGBA8: return 1;
		default: return -1;
		}
		break;
	case 'b':
		switch(format) {
		case Image::RGB8: case Image::RGBA8: return 2;
		default: return -1;
		}
		break;
	case 'a': return 1;
		switch(format) {
		case Image::RGBA8: return 3;
		default: return -1;
		}
		break;
	}
	return -1;
}

int decodeR5G6B5(const byte* p) {
	byte r = (p[1]>>3) * 255 / 31;
	byte g = ((p[1]<<3 | p[0]>>5) & 0x3f) * 255 / 63;
	byte b = (p[0]&0x1f) * 255 / 31;
	return r<<16 | g<<8 | b;
}

void createBC1Table(const byte* a, const byte* b, byte* table) {
	int ca = decodeR5G6B5(a);
	int cb = decodeR5G6B5(b);
	table[0] = ca>>16;
	table[1] = (ca>>8) & 0xff;
	table[2] = ca & 0xff;

	table[3] = cb>>16;
	table[4] = (cb>>8) & 0xff;
	table[5] = cb & 0xff;

	table[6] = (table[0] * 2 + table[3]) / 3;
	table[7] = (table[1] * 2 + table[4]) / 3;
	table[8] = (table[2] * 2 + table[5]) / 3;

	table[9]  = (table[0] + table[3] * 2) / 3;
	table[10] = (table[1] + table[4] * 2) / 3;
	table[11] = (table[2] + table[5] * 2) / 3;
}

void createBC4Table(byte a, byte b, byte* table) {
	table[0] = a;
	table[1] = b;
	if(a > b) { // Interpolate between a and b
		table[2] = (a*6 + b*1) / 7;
		table[3] = (a*5 + b*2) / 7;
		table[4] = (a*4 + b*3) / 7;
		table[5] = (a*3 + b*4) / 7;
		table[6] = (a*2 + b*5) / 7;
		table[7] = (a*1 + b*6) / 7;
	}
	else { // Interpolate but add 0% and 100% values - less used
		table[2] = (a*4 + b*1) / 5;
		table[3] = (a*3 + b*2) / 5;
		table[4] = (a*2 + b*3) / 5;
		table[5] = (a*1 + b*4) / 5;
		table[6] = 0;
		table[7] = 255;
	}
}


void decompressBC4Channel(const byte* data, int bx, int by, int stride, byte* out, int bpp) {
	byte c[8];
	int w = bx * 4;
	int line = bx * stride;
	unsigned long long b;
	for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
		const byte* block = data + x * stride + y*line;
		createBC4Table(block[0], block[1], c);
		memcpy(&b, block, 8);
		for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
			int k = i + j*4;
			int v = (b >> (48 - k*3)) & 7;
			byte* px = out + (x*4+i + (y*4+j) * w) * bpp;
			px[0] = c[v];
		}
	}
}

byte* Image::convertSurface(const byte* data, int w, int h, int d, Format to, const char* swizzle) const {
	if(isCompressed(to)) return nullptr;
	Format format = m_format;
	byte* intermediateData = nullptr;
	bool hasSwizzle = swizzle && strncmp(swizzle, "rgba", getChannels(to)) != 0;
	
	if(isCompressed(m_format) && !isCompressed(to)) { // Decompress first
		Format intermediateFormat;
		switch(m_format) {
		case BC1: intermediateFormat = RGB8; break;
		case BC2: intermediateFormat = RGBA8; break;
		case BC3: intermediateFormat = RGBA8; break;
		case BC4: intermediateFormat = R8; break;
		case BC5: intermediateFormat = getChannels(to) < 3? RG8: RGB8; break; // Optionally decode blue channel
		default: return nullptr;
		}
		byte* out = intermediateData = new byte[w*h*getBytesPerPixel(intermediateFormat)];
		byte table[12];
		const int bx = w / 4;
		const int by = h / 4;
		const int bs = format==BC1? 8: 16;
		const int line = bx * bs;

		// Colour blocks
		if(format < BC3) {
			int bpp = format == BC1? 3: 4;
			const byte* cdata = intermediateFormat==BC1? data: data+8;
			byte* cout = out + (intermediateFormat==BC1? 0: 1);
			for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
				const byte* block = cdata + x * bs + y*line;
				// decode R5G6b5 colour
				createBC1Table(block, block+2, table);
				// Decode pixels
				for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
					int p = (block[j+4]>>(i*2)) & 3;
					byte* px = cout + (x*4+i + (y*4+j) * w) * bpp;
					memcpy(px, table + p*3, 3);
				}
			}
		}
		// Single channel blocks
		switch(format) {
		case BC2: // BC2 alpha is never used, but may as well implement it
			for(int x=0; x<bx; ++x) for(int y=0; y<by; ++y) {
				const byte* block = data + x * bs + y*line;
				for(int i=0; i<4; ++i) for(int j=0; j<4; ++j) {
					int a = (block[j+4]>>(i*2)) & 3;
					byte* px = out + (x*4+i + (y*4+j) * w) * 4; // bpp=4
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
			if(intermediateFormat == RG8) {
				decompressBC4Channel(data, bx, by, 16, out, 2);
				decompressBC4Channel(data+8, bx, by, 16, out+1, 2);
			}
			else { // Probably a normal map - calculate blue channel from red,green
				decompressBC4Channel(data, bx, by, 16, out, 3);
				decompressBC4Channel(data+8, bx, by, 16, out+1, 3);
				byte* end = out + w * h * 3;
				for(byte* pixel = out; pixel<end; pixel += 3) {
					pixel[2] = 255 * sqrt(1.f - (powf(pixel[0]/255.0, 2) + powf(pixel[1]/255.f, 2)));
				}
			}
			break;
		default: break;
		}
		// Are we done, or do we need to convert further ?
		if(intermediateFormat == to && !hasSwizzle) return out;
		format = intermediateFormat;
		data = out;
	}


	// Channel changes
	int istride = getBytesPerPixel(format);
	int ostride = getBytesPerPixel(to);
	int count = w*h;
	byte* out = new byte[count * ostride];
	const byte* i = data;
	byte* o = out;
	const byte* end = data + count * istride;
	int channels = getChannels(to);
	int offsets[4] = {0,0,0,0};
	bool zeroed = false;
	if(!swizzle) swizzle = "rgba";
	for(int c=0; c<channels; ++c) {
		offsets[c] = getPixelOffset(format, swizzle[c]);
		if(offsets[c] < 0 || offsets[c] >= getChannels(format)) {
			offsets[c] = 0;
			if(!zeroed) memset(out, 0, count * istride);
			zeroed = true;
		}
	}
	while(i < end) {
		for(int c=0; c<channels; ++c) o[c] = i[offsets[c]];
		o += ostride;
		i += istride;
	}
	delete [] intermediateData;
	return out;
}

