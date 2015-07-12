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
Texture::Texture(): m_unit(0), m_width(0), m_height(0), m_depth(0) {
	initialiseTextureExtensions();
}

/** Bind texture to active unit */
void Texture::bind() const {
	glBindTexture(GL_TEXTURE_2D, m_unit);
}

/** Set raw opengl filter values */
void Texture::setFilter(unsigned min, unsigned mag) const {
	bind();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
}
/** Set enumerated filter */
void Texture::setFilter(int f) const {
	switch(f) {
	case NEAREST: setFilter(GL_LINEAR, GL_NEAREST); break;
	case BILINEAR: setFilter(GL_LINEAR, GL_LINEAR); break;
	case TRILINEAR: setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR); break;
	case ANISOTROPIC: 
		setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
		break;
	}
}

/** Set wrapping behaviour */
void Texture::setWrap(uint s, uint t) const {
	bind();
	s = s==REPEAT? GL_REPEAT: s==CLAMP? GL_CLAMP_TO_EDGE: s;
	t = t==REPEAT? GL_REPEAT: t==CLAMP? GL_CLAMP_TO_EDGE: t;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
}

/** Clear texture unit */
void Texture::destroy() {
	glDeleteTextures(1, &m_unit);
	m_width = m_height = m_depth = 0;
	m_unit = 0;
}

/** Get texture format from image bits per pixel value */
int Texture::bppFormat(int bpp) {
	static int map[] = { 0, LUMINANCE, LUMINANCE_ALPHA, RGB, RGBA };
	return map[bpp/8];
}

/** Create basic texture */
Texture Texture::create(int width, int height, int format, const void* data, bool mip) {
	// Create texture object
	Texture t;
	switch(format) {
	case ALPHA:	          t.m_depth = 1; format = GL_ALPHA; break;
	case LUMINANCE:	      t.m_depth = 1; format = GL_LUMINANCE; break;
	case LUMINANCE_ALPHA: t.m_depth = 2; format = GL_LUMINANCE_ALPHA; break;
	case RGB:             t.m_depth = 3; format = GL_RGB; break;
	case RGBA:            t.m_depth = 4; format = GL_RGBA; break;

	case GL_ALPHA:
	case GL_LUMINANCE:       t.m_depth = 1; break;
	case GL_LUMINANCE_ALPHA: t.m_depth = 2; break;
	case GL_RGB:             t.m_depth = 3; break;
	case GL_RGBA:            t.m_depth = 4; break;
	default: 
		printf("Invalid texture format\n");
		return t;
	}
	// Create texture
	glGenTextures(1, &t.m_unit);
	glBindTexture(GL_TEXTURE_2D, t.m_unit);
	// Default to bilinear texturing
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Set data
	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
	t.m_width = width; t.m_height = height;
	// Generate MipMaps
	if(mip) t.generateMipMaps(format, (const char*)data);
	return t;
}

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

/** Create texture from whatever */
Texture Texture::create(int width, int height, int format, int sFormat, int type, const void* data) {
	// Create texture object
	Texture t;
	// Create texture
	glGenTextures(1, &t.m_unit);
	glBindTexture(GL_TEXTURE_2D, t.m_unit);
	// Default to bilinear texturing
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Set data
	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, sFormat, type, data); 
	t.m_width = width; t.m_height = height;
	return t;
}

/** Create texture with pre-generated mipmaps */
Texture Texture::create1(int width, int height, int format, const ubyte*const* data, int mips) {
	// Create texture object
	Texture t;
	// Create texture
	glGenTextures(1, &t.m_unit);
	glBindTexture(GL_TEXTURE_2D, t.m_unit);
	// Default to bilinear texturing
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Compressed?
	bool compressed = format >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT && format <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	// Set data
	if(!compressed) {
		for(int i=0; i<mips+1; ++i) {
			glTexImage2D( GL_TEXTURE_2D, i, format, width>>i, height>>i, 0, format, GL_UNSIGNED_BYTE, data[i]); 
		}
	}
	else {
		int w = width, h = height;
		int blockSize = format>GL_COMPRESSED_RGBA_S3TC_DXT1_EXT? 16: 8;
		for(int i=0; i<mips+1; ++i) {
			if(w==0) w=1; if(h==0) h=1;
			int size = ((w+3)/4) * ((h+3)/4) * blockSize;
			glCompressedTexImage2D( GL_TEXTURE_2D, i, format, w, h, 0, size, data[i]); 
			w >>= 1; h >>= 1;
		}
	}
	t.m_width = width; t.m_height = height;
	return t;
}

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

