#define GL_GLEXT_PROTOTYPES
#include "base/opengl.h"
#include "base/framebuffer.h"
#include "base/game.h"

#include <cstdio>
#include <cstring>

//Extensions
#ifndef GL_VERSION_2_0
#define APIENTRYP *

#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00

typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (APIENTRYP PFNGLFRAMEBUFFERTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

PFNGLBINDRENDERBUFFERPROC	glBindRenderbuffer	= 0;
PFNGLDELETERENDERBUFFERSPROC	glDeleteRenderbuffers	= 0;
PFNGLGENRENDERBUFFERSPROC	glGenRenderbuffers	= 0;
PFNGLRENDERBUFFERSTORAGEPROC	glRenderbufferStorage	= 0;
PFNGLBINDFRAMEBUFFERPROC	glBindFramebuffer	 = 0;
PFNGLDELETEFRAMEBUFFERSPROC	glDeleteFramebuffers	 = 0;
PFNGLGENFRAMEBUFFERSPROC	glGenFramebuffers	 = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC	glCheckFramebufferStatus = 0;
PFNGLFRAMEBUFFERTURE2DPROC	glFramebufferTexture2D     = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;

int initialiseExtensions() {
	if(glBindRenderbuffer) return 1;
	glBindRenderbuffer	= (PFNGLBINDRENDERBUFFERPROC)		wglGetProcAddress("glBindRenderBuffer");
	glDeleteRenderbuffers	= PFNGLDELETERENDERBUFFERSPROC		wglGetProcAddress("glDeleteRenderbuffers");
	glGenRenderbuffers	= PFNGLGENRENDERBUFFERSPROC		wglGetProcAddress("glGenRenderbuffers");
	glRenderbufferStorage	= PFNGLRENDERBUFFERSTORAGEPROC		wglGetProcAddress("glRenderbufferStorage");
	glBindFramebuffer	= PFNGLBINDFRAMEBUFFERPROC		wglGetProcAddress("glBindFramebuffer");
	glDeleteFramebuffers	= PFNGLDELETEFRAMEBUFFERSPROC		wglGetProcAddress("glDeleteFramebuffers");
	glGenFramebuffers	= PFNGLGENFRAMEBUFFERSPROC		wglGetProcAddress("glGenFramebuffers");
	glCheckFramebufferStatus= PFNGLCHECKFRAMEBUFFERSTATUSPROC	wglGetProcAddress("glCheckFramebufferStatus");
	glFramebufferTexture2D 	= PFNGLFRAMEBUFFERTURE2DPROC		wglGetProcAddress("glFramebufferTexture2D ");
	glFramebufferRenderbuffer= PFNGLFRAMEBUFFERRENDERBUFFERPROC	wglGetProcAddress("glFramebufferRenderbuffer");
	return glBindRenderBuffer? 1: 0;
}
#else
int initialiseExtensions() { return 1; }
#endif


using namespace base;

const FrameBuffer FrameBuffer::Screen;
const FrameBuffer* FrameBuffer::s_bound = &FrameBuffer::Screen;

FrameBuffer::FrameBuffer() : m_width(0), m_height(0), m_buffer(0) {
	memset(m_colour, 0, 5*sizeof(Storage));
}
FrameBuffer::FrameBuffer(int w, int h, int f) : m_width(w), m_height(h), m_buffer(0) {
	memset(m_colour, 0, 5*sizeof(Storage));
	if(w==0 || h==0) return; //Invalid size

	// Create the frame buffer
	glGenFramebuffers(1, &m_buffer);
	if(f==0) return; //Framebuffer has no attachments

	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	const FrameBuffer* last = s_bound; //Avoid rebinding
	s_bound = this;

	//Attach stuff
	if(f&COLOUR) attachColour(TEXTURE, GL_RGBA);
	else if(f&DEPTH) attachColour(TEXTURE, GL_RGB); //Must have a colour buffer
	if(f&DEPTH)  attachDepth(TEXTURE, 24);


	if(!m_buffer || glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
		printf("Error creating frame buffer\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	s_bound = last;
}
FrameBuffer::~FrameBuffer() {
	//if(m_buffer) glDeleteFramebuffers(1, &m_buffer);
	//if(m_depth.type) glDeleteRenderbuffers(1, &m_depth.data);
}

uint FrameBuffer::attachColour(uint type, uint format, bool isFloat) {
	int index;
	for(index=0; index<4&&m_colour[index].type; index++);
	if(!format) format = GL_RGBA;
	if(type==TEXTURE) {
		m_colour[index].type = 1;
		if(isFloat) m_colour[index].texture = Texture::create(m_width, m_height, format,  GL_RGB,GL_FLOAT,0);
		else m_colour[index].texture = Texture::create(m_width, m_height, format);
		m_colour[index].data = m_colour[index].texture.unit();
		//Attach to buffer
		if(m_colour[index].texture.unit()) {
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+index, GL_TEXTURE_2D, m_colour[index].data, 0);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			return m_colour[index].data;
		}
	} else if(type==BUFFER) {
		printf("Colour renderbuffers not yet implemented\n");
	}
	printf("Error attaching colour %s %d\n", type==TEXTURE? "texture": "renderbuffer", index);
	return 0;
}
uint FrameBuffer::attachDepth(uint type, uint depth) {
	uint format;
	switch(depth) {
	case 16: format = GL_DEPTH_COMPONENT16; break;
	case 24: format = GL_DEPTH_COMPONENT24; break;
	case 32: format = GL_DEPTH_COMPONENT32; break;
	default: return 0;
	}
	if(type==TEXTURE) {
		format = GL_DEPTH_COMPONENT;
		m_depth.type = 1;
		m_depth.texture = Texture::create(m_width, m_height, format, format, GL_UNSIGNED_BYTE, 0);
		//m_depth.texture.filter(GL_NEAREST, GL_NEAREST);
		m_depth.texture.setWrap(Texture::CLAMP);
		m_depth.data = m_depth.texture.unit();
		//Attach to buffer
		if(m_depth.texture.unit()) {
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth.data, 0);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			return m_depth.data;
		}
	} else if(type==BUFFER) {
		m_depth.type = 2;
		glGenRenderbuffers(1, &m_depth.data);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth.data);
		glRenderbufferStorage(GL_RENDERBUFFER, format, m_width, m_height);
		if(m_depth.data) {
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth.data);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			return m_depth.data;
		}
	}
	printf("Error attaching depth %s (%dbit)\n", type==TEXTURE? "texture": "renderbuffer", depth);
	return 0; //Fail
}

void FrameBuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	if(m_buffer) glViewport(0,0,m_width, m_height);
	else glViewport(0, 0, Game::getSize().x, Game::getSize().y);
	s_bound = this;
}


