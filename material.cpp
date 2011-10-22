#include "material.h"

#include "opengl.h"

#include <cstdio>
#include <cstring>
#include "file.h"

#include "png.h"

//stupid windows
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

Texture::TextureMap Texture::s_textures;

const Texture& Texture::getTexture(const char* name) {
	static int e = initialiseTextureExtensions(); e=e;
	TextureMap::iterator iter = s_textures.find(name);
	if(iter == s_textures.end()) {
		printf("Loading %s\n", name);
		s_textures[name] = Texture();
		iter = s_textures.find(name);
		iter->second.m_name = strdup(name);
		//search directories
		char path[128]="";
		if(File::find(name, path)) {
			if(!iter->second.load(path)) printf("Failed to load %s\n", path);
		} else {
			if(!iter->second.load(name)) printf("Failed to load image\n");
		}
	}
	return iter->second;
}

const Texture Texture::createTexture(int width, int height, uint format, const void* data) {
	//Generate an empty texture
	uint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
	//Done
	Texture t;
	t.m_good = texture>0;
	t.m_texture = texture;
	t.m_width = width;
	t.m_height = height;
	t.m_bpp = format==GL_RGBA?32: format==GL_RGB?24: 8;
	return t;
}

const Texture& Texture::operator=(const Texture& t) {
	m_texture = t.m_texture;
	m_good = t.m_good;
	m_name = t.m_name;
	m_width = t.m_width;
	m_height = t.m_height;
	return *this;
}

int Texture::bind() const {
	if(m_good) glBindTexture(GL_TEXTURE_2D, m_texture);
	return m_good? 1:0;
}

int Texture::load(const char* filename) {
	PNG image;
	image.load(filename);
	if(!image.data) return 0;
	
	int width = image.width;
	int height = image.height;
	
	int format = GL_RGB;
	int format2 = GL_RGB;//GL_BGR_EXT;
	if(image.bpp == 32) {
		format = GL_RGBA;
		format2 = GL_RGBA;//GL_BGRA_EXT;
	}
	m_bpp = image.bpp;
	
	//create opengl texture
	uint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	//copy texture data
	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format2, GL_UNSIGNED_BYTE, image.data ); 
	
	//Done
	m_good = true;
	m_texture = texture;
	m_width = width;
	m_height = height;
	return 1;
}

void Texture::clamp(bool c) const { 
	if(m_good) {
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, c?GL_CLAMP_TO_EDGE:0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, c?GL_CLAMP_TO_EDGE:0);
	}
}
void Texture::filter(uint min, uint mag) const { 
	if(m_good) {
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	}
}
////////////////////////////////////////////////////////////////

uint Material::s_flags = 0;

Material::Material() : ambient(0x101010), shininess(50), lighting(1), depthTest(1), depthMask(1) {
}
int Material::bind() const {
	int max=0;
	for(uint i=0; i<MAX_TEXTURE_UNITS; i++) {
		if(texture[i].m_good) {
			if(i>0) glActiveTexture(GL_TEXTURE0+i);
			texture[i].bind();
			max = i;
		}
	}
	if(max) glActiveTexture(GL_TEXTURE0);
	
	//I think this only applies to fixed function
	if(texture[0].m_good) glEnable(GL_TEXTURE_2D);
	else glDisable(GL_TEXTURE_2D);
	
	//Colour
	glColor4fv( diffuse );
	
	//need to use material stuff...
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
	
	return 1;
}
