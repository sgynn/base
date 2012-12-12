#include <base/texture.h>
#include <base/opengl.h>
#include <cstdio>

#ifndef GL_CLAMP_TO_EDGE
# define GL_CLAMP_TO_EDGE 0x812f
#endif
#ifdef WIN32
PFNGLACTIVETEXTUREARBPROC glActiveTexture = 0;
int initialiseTextureExtensions() {
	if(glActiveTexture) return 1;
	glActiveTexture = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
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
void Texture::setFilter(unsigned min, unsigned mag) {
	bind();
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
}
/** Set enumerated filter */
void Texture::setFilter(int f) {
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
void Texture::setWrap(uint s, uint t) {
	bind();
	s = s==REPEAT? GL_REPEAT: s==CLAMP? GL_CLAMP_TO_EDGE: s;
	t = t==REPEAT? GL_REPEAT: t==CLAMP? GL_CLAMP_TO_EDGE: t;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
}

/** Clear texture unit */
void Texture::destroy() {
	glDeleteTextures(1, &m_unit);
}

/** Create basic texture */
Texture Texture::create(int width, int height, int format, const char* data) {
	// Create texture object
	Texture t;
	switch(format) {
	case ALPHA:	          t.m_depth = 1; format = GL_ALPHA; break;
	case LUMINANCE:	      t.m_depth = 1; format = GL_LUMINANCE; break;
	case LUMINANCE_ALPHA: t.m_depth = 2; format = GL_LUMINANCE_ALPHA; break;
	case RGB:             t.m_depth = 3; format = GL_RGB; break;
	case RGBA:            t.m_depth = 4; format = GL_RGB; break;

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
	return t;
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

