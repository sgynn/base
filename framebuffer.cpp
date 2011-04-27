#define GL_GLEXT_PROTOTYPES
#include "opengl.h"
#include "framebuffer.h"
#include "game.h"

#include <cstdio>

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

FrameBuffer FrameBuffer::Screen;

FrameBuffer::FrameBuffer() : m_width(0), m_height(0), m_buffer(0), m_depth(0) {}
FrameBuffer::FrameBuffer(int w, int h, int f) : m_width(w), m_height(h), m_buffer(0), m_depth(0) {
	if(w==0 || h==0 || f==0) return;

	// Colour buffer
	if(f&COLOUR) m_texture = Texture::createTexture(w, h, GL_RGBA);
	uint texture = m_texture.getGLTexture();

	// Depth buffer
	if(f&DEPTH) {
		glGenRenderbuffers(1, &m_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
	}

	// Frame buffer
	glGenFramebuffers(1, &m_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	if(texture) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	if(m_depth) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

	if(!m_buffer || glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
		printf("Error creating frame buffer\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
FrameBuffer::~FrameBuffer() {
	//if(m_buffer) glDeleteFramebuffers(1, &m_buffer);
	//if(m_depth) glDeleteRenderbuffers(1, &m_depth);
}

void FrameBuffer::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	if(m_buffer) glViewport(0,0,m_width, m_height);
	else glViewport(0, 0, Game::getSize().x, Game::getSize().y);
}


