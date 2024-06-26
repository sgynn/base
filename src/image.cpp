#include <base/image.h>
#include <cstring>

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
Image Image::convert(Format to) {
	int surfaces = getFaces();
	byte** data = new byte*[surfaces * m_mips];
	for(int n=0; n<surfaces; ++n) {
		int w = width, h = height, d = depth;
		for(int i=0; i<m_mips; ++i) {
			if(w<=0) w=1;
			if(h<=0) h=1;
			if(d<=0) d=1;
			
			int k = n * i * surfaces;
			data[k] = convertSurface(m_data[k], w, h, d, to);

			w = w>>1;
			h = h>>1;
			d = d>>1;
		}
	}
	return Image(to, m_width, m_height, m_depth, m_mips, data);
}

Image::byte* Image::convertSurface(byte* data, int w, int h, int d, Format to) {
	byte* out = nullptr;
	int count = w*h;
	switch(to) {
	case R8: // ---------------------------------------- //
		out = new byte[w*h];
		switch(m_format) {
		case R8: memcpy(out, data, w*h); break;
		case RG8: for(int i=0;i<count; ++i) out[i] = data[i*2]; break;
		case RGB8: for(int i=0; i<count; ++i) out[i] = data[i*3]/4 + data[i*3+1]/2 + data[i*3+2]/4; break;
		case RGBA8: for(int i=0; i<count; ++i) out[i] = data[i*4]/4 + data[i*4+1]/2 + data[i*4+2]/4; break;
		}
		break;
	}
	return out;
}
*/
