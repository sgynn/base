#include "base/font.h"
#include "base/file.h"
#include "base/opengl.h"
#include <cstdio>
#include <cstring>

#include "base/png.h"

using namespace base;

Font::Font(const char* filename) {
	char path[64]; if(File::find(filename, path)) filename = path;
	::printf("Loading font %s...", filename);
	m_tex = loadTexture(filename);
}

Font::~Font() {
	//should delete the texture;
}

//stupid windows
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812f
#endif
unsigned int Font::loadTexture(const char* filename) {
	PNG image = PNG::load(filename);
	if(!image.data) {
		::printf("Failed\n");
		return 0;
	}
	int width = image.width;
	int height = image.height;
	int format = GL_RGB;
	int format2 = GL_BGR_EXT;
	if(image.bpp == 32) {
		format = GL_RGBA;
		format2 = GL_RGBA; //GL_BGRA_EXT;
	}
	
	//extract glyph coordinates
	unsigned int g = getGlyphs(image.data, width, height, image.bpp); 
	if(g==0) { ::printf("Error: No glyphs found. Image(%d,%d)\n", width, height); /* return 0; */ } 
	
	//create opengl texture
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//copy texture data
	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format2, GL_UNSIGNED_BYTE, image.data );
	
	::printf("Done.\n");
	return texture;
}

unsigned int Font::getGlyphs(const void* data, int w, int h, int bpp) {
	memset(m_glyph, 0, 128*sizeof(Glyph));
	m_w = w; m_h=h;
	
	size_t stride = bpp/8;
	//get guide colours
	int a, b, clear;
	memcpy((char*)&a, data, stride); 				//Start: px(0,0)
	memcpy((char*)&b, (char*)data+stride, stride);			//End:   px(1,0)
	memcpy((char*)&clear, (char*)data+stride+w*stride, stride);	//Clear: px(0,1)

	::printf("Guide: %#x %#x ", a, b);
	
	//No guides specified : Fail
	if(a==b || a==clear || b==clear) return 0;
	
	//erase points
	memcpy((char*)data+stride, (char*)&clear, stride);
	
	int px, ix1=32, ix2=32, count=0; //starts at 'space'
	for(int y=0; y<h; y++) for(int x=0; x<w; x++) {
		size_t offset = x*stride + y*w*stride;
		memcpy((char*)&px, (const char*)data+offset, stride);
		if(px==a) { //Top Left Corner
			memcpy((char*)data+offset, (char*)&clear, stride);
			m_glyph[ix1].x = x;
			m_glyph[ix1].y = y;
			++ix1;
		} else if(px==b) { //Bottom Right Corner
			memcpy((char*)data+offset, (char*)&clear, stride);
			if(ix2<ix1) {
				m_glyph[ix2].w = x - m_glyph[ix2].x;
				m_glyph[ix2].h = y - m_glyph[ix2].y;
				if(m_glyph[ix2].w>0 && m_glyph[ix2].h>0) ++count;
				++ix2;
			} else {
				::printf("Error extracting character '%c' at (%d, %d)\n", ix2, x, y);
				return count;
			}
		}
	}
	return count;
}

int Font::textWidth(const char* text) const {
	int width=0;
	for(const char* c=text; *c; c++) {
		width += m_glyph[(int)*c].w;
	}
	return width;
}
int Font::textHeight(const char* text) const {
	int height=0, lineHeight=0;
	for(const char* c=text; *c; c++) {
		lineHeight = lineHeight>m_glyph[(int)*c].h? lineHeight: m_glyph[(int)*c].h;
		if(*c=='\n') { height+=lineHeight; lineHeight=0; }
	}
	return height+lineHeight;
}

#include <stdarg.h>
int Font::printf(int x, int y, const char* format, ... ) const {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	va_end (args);
	return print(x,y, buffer);
}
int Font::printf(int x, int y, float scale, const char* format, ... ) const {
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	va_end (args);
	return print(x,y,scale, buffer);
}

//Then we have to draw it....
int Font::print(int x, int y, float scale, const char* text) const {
	if(!text || !text[0]) return 0;
	int l = strlen(text);
	
	//draw it!
	glBindTexture(GL_TEXTURE_2D, m_tex);
	
	int cx=x, cy=y;
	float lh=0; //line height
	for(int i=0; i<l; i++) {
		const Glyph& glyph = m_glyph[ (int)text[i] ];
		
		//scale character
		float nw = glyph.w * scale;
		float nh = glyph.h * scale;
		lh = lh>nh? lh: nh;
		
		if(glyph.w > 0 && glyph.h > 0) {
			//get texture coordinates
			float tx = (float)glyph.x / m_w;
			float ty = (float)glyph.y / m_h;
			float tx2 = tx + (float)glyph.w / m_w;
			float ty2 = ty + (float)glyph.h / m_h;
			
			//draw character
			glBegin(GL_QUADS);
			glTexCoord2f(tx,  ty2);	glVertex2f(	cx,	cy);
			glTexCoord2f(tx2, ty2);	glVertex2f(	cx+nw,	cy);
			glTexCoord2f(tx2, ty );	glVertex2f(	cx+nw,	cy+nh);
			glTexCoord2f(tx,  ty );	glVertex2f(	cx,	cy+nh);
			glEnd();
		}
		
		//next character position
		cx += (int) nw;
		if(text[i]=='\n') {
			cx = x;
			cy -= (int) lh;
			lh=0;
		}
	}
	
	return 1;
}

