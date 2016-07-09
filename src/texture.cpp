#include <base/opengl.h>
#include <base/texture.h>
#include <cstdio>

#ifndef GL_CLAMP_TO_EDGE
# define GL_CLAMP_TO_EDGE 0x812f
# define GL_TEXTURE0      0x84C0
#endif

#ifndef GL_VERSION_2_0
#define GL_TEXTURE_1D_ARRAY	 0x8C18
#define GL_TEXTURE_2D_ARRAY  0x8C1A
#define GL_R32F              0x822E
#define GL_RG32F             0x8230
#define GL_RGB32F            0x8815
#define GL_RGBA32F           0x8814
#define GL_R16F              0x822D
#define GL_RG16F             0x822F
#define GL_RGB16F            0x881B
#define GL_RGBA16F           0x881A
#define GL_RGB565            0x8D62
#define GL_R11F_G11F_B10F    0x8C3A
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_DEPTH24_STENCIL8  0x88F0
#define GL_HALF_FLOAT        0x140B

#define APIENTRYP __stdcall *
typedef void (APIENTRYP PFNGLGENERATEMIPMAPPROC) (GLenum target);
#endif

#ifdef WIN32
#define GL_TEXTURE_1D_ARRAY	 0x8C18
#define GL_TEXTURE_2D_ARRAY  0x8C1A
#define GL_R32F              0x822E
#define GL_RG32F             0x8230
#define GL_RGB32F            0x8815
#define GL_RGBA32F           0x8814
#define GL_R16F              0x822D
#define GL_RG16F             0x822F
#define GL_RGB16F            0x881B
#define GL_RGBA16F           0x881A
#define GL_RGB565            0x8D62
#define GL_R11F_G11F_B10F    0x8C3A
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_DEPTH24_STENCIL8  0x88F0
#define GL_HALF_FLOAT        0x140B

#define APIENTRYP __stdcall *
typedef void (APIENTRYP PFNGLGENERATEMIPMAPPROC) (GLenum target);
PFNGLACTIVETEXTUREARBPROC glActiveTexture = 0;
PFNGLTEXIMAGE3DPROC glTexImage3D = 0;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D = 0;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = 0;
int initialiseTextureExtensions() {
	if(glActiveTexture) return 1;
	glActiveTexture        = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	glTexImage3D           = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
	glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)wglGetProcAddress("glCompressedTexImage1D");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
	glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)wglGetProcAddress("glCompressedTexImage3D");
	glGenerateMipmap       = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
	return glActiveTexture? 1: 0;
}
#else
int initialiseTextureExtensions() { return 1; }
#endif



using namespace base;

/** Constructor */
Texture::Texture(): m_unit(0), m_type(TEX2D), m_format(NONE), m_hasMipMaps(false), m_width(0), m_height(0), m_depth(0) {
	initialiseTextureExtensions();
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
	static const unsigned glv[] = { GL_REPEAT, GL_CLAMP_TO_EDGE };
	bind();
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_S, glv[s]);
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_T, glv[t]);
	glTexParameteri(getTarget(), GL_TEXTURE_WRAP_R, glv[r]);
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
		0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
		GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F,
		GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F,
		GL_R11F_G11F_B10F, GL_RGB565,
		GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH24_STENCIL8
	};
	return formats[f];
}
unsigned Texture::getInternalDataType(Format f) {
	if(f == NONE)    return 0;
	if(f <= RGBA8)   return GL_UNSIGNED_BYTE;
	if(f <= DXT5)    return GL_UNSIGNED_BYTE;	// Not accurare
	if(f <= RGBA32F) return GL_FLOAT;
	if(f <= RGBA16F) return GL_HALF_FLOAT;
	if(f <= D24S8)   return GL_UNSIGNED_BYTE;
	return 0;
}
bool Texture::isCompressedFormat(Format f) {
	return f==DXT1 || f==DXT3 || f==DXT5;
}



// ----------------------------------------------------------------------------------------------------- //

unsigned Texture::getMemorySize(Format format, int w, int h, int d) {
	switch(format) {
	case NONE:     return 0;
	case R8:       return w*h*d;
	case RG8:      return w*h*d*2;
	case RGB8:     return w*h*d*3;
	case RGBA8:    return w*h*d*4;
	case DXT1:     return ((w+3)/4) * ((h+3)/4) * 8 * d;
	case DXT3:
	case DXT5:     return ((w+3)/4) * ((h+3)/4) * 16 * d;
	case R32F:     return w*h*d*4;
	case RG32F:    return w*h*d*8;
	case RGB32F:   return w*h*d*12;
	case RGBA32F:  return w*h*d*16;
	case R16F:     return w*h*d*2;
	case RG16F:    return w*h*d*4;
	case RGB16F:   return w*h*d*6;
	case RGBA16F:  return w*h*d*8;
	case R11G11G10F: return w*h*d*4;
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
		printf("Invalid texture format\n");
		return Texture();
	}
	
	// data format may be different
	unsigned dfmt = fmt;
	if(format == D16 || format == D24 || format == D32) dfmt = GL_DEPTH_COMPONENT;
	else if(format == D24S8) dfmt = GL_DEPTH_STENCIL;



	Texture t;
	t.m_type   = type;
	t.m_format = format;
	t.m_width  = width;
	t.m_height = height;
	t.m_depth  = depth;
	unsigned target = t.getTarget();
	unsigned ftype = getInternalDataType(format);

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
			case TEX1D: glCompressedTexImage1D(target, mip, fmt, width, 0, size, src); break;
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
			case TEX1D: glTexImage1D(target, mip, fmt, width, 0, dfmt, ftype, src); break;
			case ARRAY1D:
			case TEX2D: glTexImage2D(target, mip, fmt, width, height, 0, dfmt, ftype, src); break;
			case ARRAY2D:
			case TEX3D: glTexImage3D(target, mip, fmt, width, height, depth, 0, dfmt, ftype, src); break;
			case CUBE: 
				for(int face=0; face<6; ++face)
					glTexImage2D(GL_TEXTURE_2D, mip, fmt, width, height, 0, dfmt, ftype, data? data[face+6*mip]: 0);
				break;
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

