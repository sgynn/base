#include <base/opengl.h>
#include <base/texture.h>
#include <cstdio>

using namespace base;

/** Constructor */
Texture::Texture(): m_unit(0), m_type(TEX2D), m_format(NONE), m_hasMipMaps(false), m_width(0), m_height(0), m_depth(0) {
}

Texture::Texture(uint glUnit, Type type) : m_unit(glUnit), m_type(type), m_format(NONE), m_hasMipMaps(false), m_width(0), m_height(0), m_depth(0) {
	bind();
	int fmt = 0;
	unsigned target = getTarget();
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &m_width);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &m_height);
	glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
	for(int f=NONE; f<D24S8; ++f) {
		if(getInternalFormat((Format)f) == (uint)fmt) m_format = (Format)f;
	}
}

/** Get bind target */
inline unsigned Texture::getTarget() const {
	static unsigned targets[] = { GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY };
	return targets[m_type];
}

/** Bind texture to active unit */
void Texture::bind() const {
	glBindTexture( getTarget(), m_unit );
}

/** Bind texture to specified unit */
void Texture::bind(int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	bind();
	glActiveTexture(GL_TEXTURE0);
}

/** Set raw opengl filter values */
void Texture::setFilter(unsigned min, unsigned mag) const {
	bind();
	glTexParameteri(getTarget(), GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(getTarget(), GL_TEXTURE_MAG_FILTER, mag);
}
/** Set enumerated filter */
void Texture::setFilter(Filter f) const {
	switch(f) {
	case NEAREST:    setFilter(GL_LINEAR, GL_NEAREST); break;
	case BILINEAR:   setFilter(GL_LINEAR, GL_LINEAR); break;
	case TRILINEAR:  setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR); break;
	case ANISOTROPIC: 
		setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
		break;
	}
}

/** Set wrapping behaviour */
void Texture::setWrap(Wrapping s, Wrapping t, Wrapping r) const {
	static const unsigned glv[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER };
	bind();
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_S, glv[s]);
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_T, glv[t]);
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_R, glv[r]);
}

void Texture::setBorderColour(float* rgba) const {
	bind();
	glTexParameterfv(getTarget(), GL_TEXTURE_BORDER_COLOR, rgba);
}

/** Clear texture unit */
void Texture::destroy() {
	if(m_format == NONE) return;	// doesnt exist
	glDeleteTextures(1, &m_unit);
	m_width = m_height = m_depth = 0;
	m_format = NONE;
	m_unit = 0;
}

/** Convert format enum to opengl value */
unsigned Texture::getInternalFormat(Format f) {
	static unsigned formats[] = {
		0, GL_RED, GL_RG, GL_RGB, GL_RGBA,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, // BC1,2,3
		GL_COMPRESSED_RED_RGTC1, GL_COMPRESSED_RG_RGTC2,	// BC4, BC5
		GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F,
		GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F,
		GL_R11F_G11F_B10F, GL_RGB565,
		GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH24_STENCIL8
	};
	return formats[f];
}
unsigned Texture::getDataFormat(Format f) {
	static unsigned formats[] = { 
		0, GL_RED, GL_RG, GL_RGB, GL_RGBA,
		0,0,0,0,0, // not applicable for compressed formats
		GL_RED, GL_RG, GL_RGB, GL_RGBA,
		GL_RED, GL_RG, GL_RGB, GL_RGBA,
		GL_RGB, GL_RGB,
		GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL
	};
	return formats[f];
}
unsigned Texture::getDataType(Format f) {
	if(f == NONE)    return 0;
	if(f <= RGBA8)   return GL_UNSIGNED_BYTE;
	if(f <= BC5)     return GL_UNSIGNED_BYTE;	// Not accurate due to block compression
	if(f <= RGBA32F) return GL_FLOAT;
	if(f <= RGBA16F) return GL_HALF_FLOAT;
	if(f <= D32)     return GL_UNSIGNED_BYTE;
	if(f == D24S8)   return GL_UNSIGNED_INT_24_8;
	return 0;
}
bool Texture::isCompressedFormat(Format f) {
	return f >= BC1 && f <= BC5;
}



// ----------------------------------------------------------------------------------------------------- //

unsigned Texture::getMemorySize(Format format, int w, int h, int d) {
	switch(format) {
	case NONE:     return 0;
	case R8:       return w*h*d;
	case RG8:      return w*h*d*2;
	case RGB8:     return w*h*d*3;
	case RGBA8:    return w*h*d*4;
	case BC1:      return ((w+3)/4) * ((h+3)/4) * 8 * d;	// compressed 4 bits per pixel
	case BC2:
	case BC3:      return ((w+3)/4) * ((h+3)/4) * 16 * d;	// compressed 1 byte per pixel
	case BC4:      return ((w+3)/4) * ((h+3)/4) * 8  * d;
	case BC5:      return ((w+3)/4) * ((h+3)/4) * 16 * d;
	case R32F:     return w*h*d*4;
	case RG32F:    return w*h*d*8;
	case RGB32F:   return w*h*d*12;
	case RGBA32F:  return w*h*d*16;
	case R16F:     return w*h*d*2;
	case RG16F:    return w*h*d*4;
	case RGB16F:   return w*h*d*6;
	case RGBA16F:  return w*h*d*8;
	case R11G11B10F: return w*h*d*4;
	case R5G6B5:     return w*h*d*2;
	case D16:      return w*h*d*2;
	case D24:      return w*h*d*3;
	case D32:      return w*h*d*4;
	case D24S8:    return w*h*d*4;
	}
	return 0;
}


/** Create basic texture */
Texture Texture::create(Type type, int width, int height, int depth,  Format format, const void*const* data, int mipmaps) {
	// Create texture object
	unsigned fmt = getInternalFormat(format);
	if(fmt == 0) {
		printf("Error: Invalid texture format %d\n", (int)format);
		return Texture();
	}
	
	Texture t;
	t.m_type   = type;
	t.m_format = format;
	t.m_width  = width;
	t.m_height = height;
	t.m_depth  = depth;
	unsigned target = t.getTarget();
	unsigned dfmt  = getDataFormat(format);
	unsigned ftype = getDataType(format);

	// Create texture
	glGenTextures(1, &t.m_unit);
	glBindTexture(target, t.m_unit);

	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmaps);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mipmaps>1? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Copy memory
	if(isCompressedFormat(format)) {
		for(int mip=0; mip<mipmaps; ++mip) {
			const void* src = data? data[mip]: 0;
			size_t size = getMemorySize(format, width, height, depth);
			switch(type) {
			#ifndef EMSCRIPTEN
			case TEX1D: glCompressedTexImage1D(target, mip, fmt, width, 0, size, src); break;
			#endif
			case ARRAY1D:
			case TEX2D: glCompressedTexImage2D(target, mip, fmt, width, height, 0, size, src); break;
			case ARRAY2D:
			case TEX3D: glCompressedTexImage3D(target, mip, fmt, width, height, depth, 0, size, src); break;
			case CUBE:
				for(int face=0; face<6; ++face)
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, mip, fmt, width, height, 0, size, data? data[face + mip*6]: 0);
				break;
			default:
				break;
			}

			GL_CHECK_ERROR;
			if(width>1) width >>=1;
			if(height>1 && type != ARRAY1D) height >>= 1;
			if(depth>1 && type != ARRAY2D)  depth >>= 1;
		}
	} else {
		for(int mip=0; mip<mipmaps; ++mip) {
			const void* src = data? data[mip]: 0;
			switch(type) {
			#ifndef EMSCRIPTEN
			case TEX1D: glTexImage1D(target, mip, fmt, width, 0, dfmt, ftype, src); break;
			#endif
			case ARRAY1D:
			case TEX2D: glTexImage2D(target, mip, fmt, width, height, 0, dfmt, ftype, src); break;
			case ARRAY2D:
			case TEX3D: glTexImage3D(target, mip, fmt, width, height, depth, 0, dfmt, ftype, src); break;
			case CUBE: 
				for(int face=0; face<6; ++face)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, mip, fmt, width, height, 0, dfmt, ftype, data? data[face+6*mip]: 0);
				break;
			default: break;
			}

			GL_CHECK_ERROR;
			if(width>1) width >>=1;
			if(height>1 && type != ARRAY1D) height >>= 1;
			if(depth>1 && type != ARRAY2D)  depth >>= 1;
		}
	}

	return t;
}

Texture Texture::create(Type type, int w, int h, int d, Format f, const void* data, bool genmips) {
	Texture t = create(type, w, h, d, f, &data, 1);
	// Generate mipmaps
	if(genmips && t.m_format==f) {
		glTexParameteri(t.getTarget(), GL_TEXTURE_MAX_LEVEL, (int)log2(w<h? w: h));
		glGenerateMipmap( t.getTarget() );
		// t.generateMipMaps(format, data);	// cpu version
	}
	return t;
}

Texture Texture::create(int w, int h, Format f, const void* data, bool genMips) {
	return create(TEX2D, w, h, 1, f, data, genMips);
}

Texture Texture::create(int w, int h, int channels, const void* data, bool genMips) {
	if(channels<1 || channels > 4) return Texture();
	return create(TEX2D, w, h, 1, (Format) channels, data, genMips);
}


// ----------------------------------------------------------------------------------------------------- //


// ToDo: setPixels() functions

int Texture::setPixels(int width, int height, Format format, const void* src, int mip) {
	unsigned target = getTarget();
	glBindTexture(target, m_unit);
	unsigned fmt = getInternalFormat(format);
	int depth = 1;
	if(isCompressedFormat(format)) {
		size_t size = getMemorySize(format, width, height, depth);
		switch(m_type) {
		#ifndef EMSCRIPTEN
		case TEX1D: glCompressedTexImage1D(target, mip, fmt, width, 0, size, src); break;
		#endif
		case ARRAY1D:
		case TEX2D: glCompressedTexImage2D(target, mip, fmt, width, height, 0, size, src); break;
		case ARRAY2D:
		case TEX3D: glCompressedTexImage3D(target, mip, fmt, width, height, depth, 0, size, src); break;
		case CUBE: break;
		default:
			break;
		}
	} else {
		// Depth formats need different enum value
		unsigned dfmt = getDataFormat(format);
		unsigned dtype = getDataType(format);

		switch(m_type) {
		#ifndef EMSCRIPTEN
		case TEX1D: glTexImage1D(target, mip, fmt, width, 0, dfmt, dtype, src); break;
		#endif
		case ARRAY1D:
		case TEX2D: glTexImage2D(target, mip, fmt, width, height, 0, dfmt, dtype, src); break;
		case ARRAY2D:
		case TEX3D: glTexImage3D(target, mip, fmt, width, height, depth, 0, dfmt, dtype, src); break;
		case CUBE: break;
		default: break;
		}
	}
	GL_CHECK_ERROR;
	return 1;
}

int Texture::setPixels(int width, int height, int layer, Format format, const void* data, int mip) {
	if(format != m_format || m_type!=ARRAY2D || m_type!=CUBE) return 0;
	unsigned target = getTarget();
	glBindTexture(target, m_unit);
	unsigned fmt = getInternalFormat(format);
	int depth = 1;
	if(isCompressedFormat(format)) {
		size_t size = getMemorySize(format, width, height, depth);
		switch(m_type) {
		case ARRAY2D:
		case TEX3D: glCompressedTexSubImage3D(target, mip, 0,0,layer, width, height, 1, fmt, size, data); break;
		case CUBE: glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+layer, mip, fmt, width, height, 0, size, data); break;
		default: break;
		}
	} else {
		// Depth formats need different enum value
		unsigned dfmt = getDataFormat(format);
		unsigned dtype = getDataType(format);

		switch(m_type) {
		case ARRAY2D:
		case TEX3D: glTexSubImage3D(target, mip, 0,0,layer, width, height, 1, dfmt, dtype, data); break;
		case CUBE: glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+layer, mip, fmt, width, height, 0, dfmt, dtype, data); break;
		default: break;
		}
	}
	GL_CHECK_ERROR;
	return 1;
}

int Texture::setPixels(int x, int y, int w, int h, Format format, const void* src, int mip) {
	if(m_format != format || m_type != TEX2D) return 0; // Must be same format
	unsigned target = getTarget();
	glBindTexture(target, m_unit);
	unsigned fmt = getInternalFormat(format);
	if(isCompressedFormat(format)) {
		size_t size = getMemorySize(format, w, h, 1);
		glCompressedTexSubImage2D(target, mip, x, y, w, h, fmt, size, src);
	}
	else {
		unsigned dfmt = getDataFormat(format);
		unsigned dtype = getDataType(format);
		glTexImage2D(target, mip, x, y, w, h, dfmt, dtype, src);
	}
	GL_CHECK_ERROR;
	return 1;
}

// ----------------------------------------------------------------------------------------------------- //


//@deprecated - use hardware generation instead (opengl3+)
int Texture::generateMipMaps(int format, const void* source) {
	int w = m_width>>1;
	int h = m_height>>1;
	int bpp = m_depth;
	int sl = m_width * bpp;
	unsigned char* buffer = new unsigned char[w*h*bpp + w*h*bpp/4];
	unsigned char* sbuffer = buffer + w*h*bpp;
	const unsigned char* s = (const unsigned char*) source;
	unsigned char* t = buffer;
	int level = 1;
	while(w>0 && h>0) {
		// Sample level above
		for(int x=0; x<w; ++x) for(int y=0; y<h; ++y) for(int z=0; z<bpp; ++z) {
			int k = z + x*2*bpp + y*2*sl;	// first pixel address
			t[z+(x+y*w)*bpp] = (s[k] + s[k+bpp] + s[k+sl] + s[k+sl+bpp]) / 4;
		}
		glTexImage2D( GL_TEXTURE_2D, level, format, w, h, 0, format, GL_UNSIGNED_BYTE, t);
		// Setup values for next level
		t = t==buffer? sbuffer: buffer;
		s = s==buffer? sbuffer: buffer;
		sl=w*bpp; w>>=1; h>>=1; ++level;
	}
	delete [] buffer;
	return level;
}


// ==================================================================================================== //


/** Material bind function */
void SMaterial::bind() const {
	// Bind textures
	int max = -1;
	for(uint i=0; i<MAX_TEXTURE_UNITS; i++) {
		if(texture[i].unit()) {
			if(i>0) glActiveTexture(GL_TEXTURE0+i);
			texture[i].bind();
			max = i;
		}
	}
	if(max) glActiveTexture(GL_TEXTURE0);
	
	// Enable/disable texturing (doesn't apply to shaders);
	if(max>=0) glEnable(GL_TEXTURE_2D);
	else glDisable(GL_TEXTURE_2D);
	
	// Set colour
	glColor4fv( diffuse );
	
	// Set material properties
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  specular);
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

