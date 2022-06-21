#include "base/opengl.h"
#include "base/framebuffer.h"
#include "base/game.h"

#include <cstdio>
#include <cstring>

using namespace base;

FrameBuffer FrameBuffer::s_screen;
const FrameBuffer& FrameBuffer::Screen = FrameBuffer::s_screen;
const FrameBuffer* FrameBuffer::s_bound = &FrameBuffer::Screen;

void FrameBuffer::setScreenSize(int w, int h) {
	s_screen.m_width = w;
	s_screen.m_height = h;
}

FrameBuffer::FrameBuffer() : m_width(0), m_height(0), m_buffer(0), m_count(0) {
}
FrameBuffer::FrameBuffer(int w, int h, int f) : m_width(w), m_height(h), m_buffer(0), m_count(0) {
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


