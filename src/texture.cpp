#include <base/texture.h>
#include <base/opengl.h>
#include <cstdio>

#ifndef GL_CLAMP_TO_EDGE
# define GL_CLAMP_TO_EDGE 0x812f
# define GL_TEXTURE0      0x84C0
#endif
#ifdef WIN32
PFNGLACTIVETEXTUREARBPROC glActiveTexture = 0;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = 0;
int initialiseTextureExtensions() {
	if(glActiveTexture) return 1;
	glActiveTexture = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
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
	glTexParameterf(getTarget(), GL_TEXTURE_MIN_FILTER, min);
	glTexParameterf(getTarget(), GL_TEXTURE_MAG_FILTER, mag);
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
	glTexParameterf(getTarget(), GL_TEXTURE_WRAP_S, glv[s]);
	glTexParameterf(getTarget(), GL_TEXTURE_WRAP_T, glv[t]);
	glTexParameterf(getTarget(), GL_TEXTURE_WRAP_R, glv[r]);
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
unsigned Texture::convertFormat(Format f) {
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
unsigned Texture::getDataFormat(Format f) {
	if(f == NONE)    return 0;
	if(f <= RGBA8)   return GL_UNSIGNED_BYTE;
	if(f <= DXT5)    return GL_UNSIGNED_BYTE;	// Not accurare
	if(f <= RGBA32F) return GL_FLOAT;
	if(f <= RGBA16F) return GL_HALF_FLOAT;
	if(f <= D24S8)   return GL_UNSIGNED_BYTE;
	return 0;
}
bool Texture::compressedFormat(Format f) {
	return f==DXT1 || f==DXT3 || f==DXT5;
}



// ----------------------------------------------------------------------------------------------------- //



/** Create basic texture */
Texture Texture::create(Type type, int width, int height, int depth,  Format format, const void*const* data, int mipmaps) {
	// Create texture object
	unsigned fmt = convertFormat(format);
	if(fmt == 0) {
		printf("Invalid texture format\n");
		return Texture();
	}

	Texture t;
	t.m_type   = type;
	t.m_format = format;
	t.m_width  = width;
	t.m_height = height;
	t.m_depth  = depth;
	unsigned target = t.getTarget();
	unsigned ftype = getDataFormat(format);

	// Create texture
	glGenTextures(1, &t.m_unit);
	glBindTexture(target, t.m_unit);

	// Set data
	switch(type) {
	case TEX1D:
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i;
			glTexImage1D( target, i, fmt, w?w:1, 0, fmt, ftype, data[i]); break;
		}
	case TEX2D:
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i, h = height>>i;
			if(compressedFormat(format)) {
				int blockSize = format == DXT1? 8: 16;
				int size = (((w?w:1)+3)/4) * (((h?h:1)+3)/4) * blockSize;
				glCompressedTexImage2D( target, i, fmt, w?w:1, h?h:1, 0, size, data[i]); 
			} else {
				glTexImage2D( target, i, fmt, w?w:1, h?h:1, 0, fmt, ftype, data[i]);
			}
		}
		break;
		
	case TEX3D:
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i, h = height>>i, d = depth>>i;
			glTexImage3D( target, 0, fmt, w?w:1, h?h:1, d?d:1, 0, fmt, ftype, data);
		}
		break;

	case ARRAY1D:
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i;
			glTexImage2D( target, i, fmt, w?w:1, height, 0, fmt, ftype, data[i]);
		}
		break;

	case ARRAY2D:
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i, h = height>>i;
			glTexImage3D( target, 0, fmt, w?w:1, h?h:1, depth, 0, fmt, ftype, data);
		}
		break;

	case CUBE:
		// Needs 6*mipmaps surfaces
		for(int i=0; i<mipmaps; ++i) {
			int w = width>>i, h = height>>i;
			if(compressedFormat(format)) {
				int blockSize = format == DXT1? 8: 16;
				int size = (((w?w:1)+3)/4) * (((h?h:1)+3)/4) * blockSize;
				for(int j=0; j<6; ++j)
					glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X+j, i, fmt, w?w:1, h?h:1, 0, size, data[i*6+j]); 
			} else {
				for(int j=0; j<6; ++j)
					glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i, fmt, w?w:1, h?h:1, 0, fmt, ftype, data[i*6+j]);
			}
		}
		break;
	}

	// Initial filtering
	glTexParameterf(target, GL_TEXTURE_MIN_FILTER, mipmaps>1? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR);
	glTexParameterf(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

