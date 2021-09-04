#include "base/opengl.h"
#include "base/framebuffer.h"
#include "base/game.h"

#include <cstdio>
#include <cstring>

//Extensions
#ifndef GL_VERSION_2_0

#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_DEPTH24_STENCIL8               0x88F0

typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLDRAWBUFFERSPROC)(GLsizei n, const GLenum* bufs);
#endif
#ifndef GL_VERSION_3_0
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
#endif

#ifdef WIN32
PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer        = 0;
PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers     = 0;
PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers        = 0;
PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage     = 0;
PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer         = 0;
PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers      = 0;
PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers         = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus  = 0;
PFNGLFRAMEBUFFERTEXTURE2DPROC     glFramebufferTexture2D    = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;
PFNGLDRAWBUFFERSPROC             glDrawBuffers = 0;

int initialiseFBOExtensions() {
	if(glBindRenderbuffer) return 1;
	glBindRenderbuffer        = (PFNGLBINDRENDERBUFFERPROC)			wglGetProcAddress("glBindRenderbuffer");
	glDeleteRenderbuffers     = (PFNGLDELETERENDERBUFFERSPROC)		wglGetProcAddress("glDeleteRenderbuffers");
	glGenRenderbuffers        = (PFNGLGENRENDERBUFFERSPROC)			wglGetProcAddress("glGenRenderbuffers");
	glRenderbufferStorage     = (PFNGLRENDERBUFFERSTORAGEPROC)		wglGetProcAddress("glRenderbufferStorage");
	glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)			wglGetProcAddress("glBindFramebuffer");
	glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)		wglGetProcAddress("glDeleteFramebuffers");
	glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)			wglGetProcAddress("glGenFramebuffers");
	glCheckFramebufferStatus  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)	wglGetProcAddress("glCheckFramebufferStatus");
	glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)		wglGetProcAddress("glFramebufferTexture2D");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)	wglGetProcAddress("glFramebufferRenderbuffer");
	glDrawBuffers             = (PFNGLDRAWBUFFERSPROC)              wglGetProcAddress("glDrawBuffers");
	if(glBindFramebuffer==0) printf("Error: Framebuffers not initialised\n");
	return glBindRenderbuffer? 1: 0;
}
#else
int initialiseFBOExtensions() { return 1; }
#endif


using namespace base;

const FrameBuffer FrameBuffer::Screen;
const FrameBuffer* FrameBuffer::s_bound = &FrameBuffer::Screen;

FrameBuffer::FrameBuffer() : m_width(0), m_height(0), m_buffer(0), m_count(0) {
	memset(m_colour, 0, 4*sizeof(Storage));
}
FrameBuffer::FrameBuffer(int w, int h, int f) : m_width(w), m_height(h), m_buffer(0), m_count(0) {
	initialiseFBOExtensions();
	memset(m_colour, 0, 4*sizeof(Storage));
	if(w==0 || h==0) return; //Invalid size

	// Create the frame buffer
	glGenFramebuffers(1, &m_buffer);
	if(f==0) return; //Framebuffer has no attachments

	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	const FrameBuffer* last = s_bound; //Avoid rebinding
	s_bound = this;

	//Attach stuff
	if(f) {
		if(f&COLOUR) attachColour(TEXTURE, Texture::RGBA8);
		else {
			//glDrawBuffer(GL_NONE);
			//glReadBuffer(GL_NONE);
		}
		if(f&DEPTH)  attachDepth(TEXTURE, Texture::D24S8);
	}


	if(!m_buffer || glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
		printf("Error creating frame buffer\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if(isDepthOnly()) {
		//glDrawBuffer(GL_BACK);
		//glReadBuffer(GL_BACK);
	}
	s_bound = last;
}
FrameBuffer::~FrameBuffer() {
	//if(m_buffer) glDeleteFramebuffers(1, &m_buffer);
	//if(m_depth.type) glDeleteRenderbuffers(1, &m_depth.data);
}

uint FrameBuffer::attachColour(uint type, Texture::Format format) {
	int index = m_count;
	if(!format) format = Texture::RGBA8;
	if(type==TEXTURE) {
		m_colour[index].type = 1;
		m_colour[index].texture = Texture::create(m_width, m_height, format);
		m_colour[index].data = m_colour[index].texture.unit();
		//Attach to buffer
		if(m_colour[index].texture.unit()) {
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+index, GL_TEXTURE_2D, m_colour[index].data, 0);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			++m_count;
			return m_colour[index].data;
		}
	} else if(type==BUFFER) {
		printf("Colour renderbuffers not yet implemented\n");
	}
	printf("Error attaching colour %s %d\n", type==TEXTURE? "texture": "renderbuffer", index);
	return 0;
}
uint FrameBuffer::attachDepth(uint type, Texture::Format format) {
	if(format < Texture::D16 || format > Texture::D24S8) return 0; // invalid format
	if(type==TEXTURE) {
		m_depth.type = 1;
		m_depth.texture = Texture::create(m_width, m_height, format);
		//m_depth.texture.filter(GL_NEAREST, GL_NEAREST);
		m_depth.texture.setWrap(Texture::CLAMP);
		m_depth.data = m_depth.texture.unit();
		//Attach to buffer
		if(m_depth.texture.unit()) {
			GLuint attachment = format == Texture::D24S8? GL_DEPTH_STENCIL_ATTACHMENT: GL_DEPTH_ATTACHMENT;
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, m_depth.data, 0);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			return m_depth.data;
		}
	} else if(type==BUFFER) {
		// Get openGL internal format
		static GLuint fmt[] = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH24_STENCIL8 };
		int glformat = fmt[ format - Texture::D16 ];
		
		m_depth.type = 2;
		glGenRenderbuffers(1, &m_depth.data);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth.data);
		glRenderbufferStorage(GL_RENDERBUFFER, glformat, m_width, m_height);
		if(m_depth.data) {
			GLuint attachment = format == Texture::D24S8? GL_DEPTH_STENCIL_ATTACHMENT: GL_DEPTH_ATTACHMENT;
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, m_depth.data);
			if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
			return m_depth.data;
		}
	}
	printf("Error attaching depth %s\n", type==TEXTURE? "texture": "renderbuffer");
	return 0; //Fail
}

bool FrameBuffer::isValid() const {
	return glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
}

int FrameBuffer::width() const {
	return m_buffer? m_width: Game::width();
}
int FrameBuffer::height() const {
	return m_buffer? m_height: Game::height();
}

bool FrameBuffer::bind(const Rect& r) const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
	if(m_buffer) {
		glViewport(r.x,r.y,r.width, r.height);
		// Disable colour drawing if depth only
		if( isDepthOnly() ) {
			glDrawBuffers(0, GL_NONE);
			//glReadBuffer(GL_NONE);
		}
		else {
			static GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_count, buffers);
		}
	}
	// Null frame buffer
	else {
		glViewport(r.x,r.y,r.width, r.height);
		if(s_bound->isDepthOnly()) {
			static GLenum buffers[] = { GL_BACK };
			glDrawBuffers(1, buffers);
			//glReadBuffer(GL_BACK);
		}
	}
	s_bound = this;
	return m_buffer || this==&Screen;
}

bool FrameBuffer::bind() const {
	return bind( Rect(0, 0, width(), height()) );
}


