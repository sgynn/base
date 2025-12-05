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

FrameBuffer::FrameBuffer() {
}

FrameBuffer::FrameBuffer(int w, int h) : m_width(w), m_height(h) {
	glGenFramebuffers(1, &m_buffer);
	if(!m_buffer) printf("Error creating frame buffer\n");
}

FrameBuffer::~FrameBuffer() {
	if(m_depth.type == RENDERBUFFER) glDeleteRenderbuffers(1, &m_depth.data);
	if(m_buffer) glDeleteFramebuffers(1, &m_buffer);
}

void FrameBuffer::attachTexture(Texture::Format fmt) {
	if(m_width > 0 && m_height > 0 && fmt != Texture::NONE) {
		if(Texture::isDepthFormat(fmt)) {
			if(m_depth.type) printf("Warning: Framebuffer already has a depth attachment\n");
			attachDepth(fmt);
		}
		else attachColour(fmt);
	}
	else {
		printf("Error: Invalid framebuffer texture\n");
	}
}

void FrameBuffer::attachColour(uint index, const Texture& texture) {
	if(!m_buffer) {
		printf("Invalid framebuffer\n");
	}
	else if(texture.unit()) {
		m_colour[index].type = TEXTURE;
		m_colour[index].texture = texture;
		m_colour[index].data = texture.unit();
		if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+index, GL_TEXTURE_2D, m_colour[index].data, 0);
		if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
		if(index >= m_count) m_count = index + 1;
		if(index==0) {
			m_width = texture.width();
			m_height = texture.height();
		}
	}
}
void FrameBuffer::attachDepth(const Texture& texture) {
	if(!m_buffer) {
		printf("Invalid framebuffer\n");
	}
	else if(!Texture::isDepthFormat(texture.getFormat())) {
		printf("Texture is not a depth format\n");
	}
	else {
		m_depth.type = TEXTURE;
		m_depth.texture = texture;
		m_depth.data = texture.unit();
		if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
		GLuint attachment = texture.getFormat() == Texture::D24S8? GL_DEPTH_STENCIL_ATTACHMENT: GL_DEPTH_ATTACHMENT;
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, m_depth.data, 0);
		if(s_bound!=this) glBindFramebuffer(GL_FRAMEBUFFER, s_bound->m_buffer);
	}
}

uint FrameBuffer::attachColour(Texture::Format format) {
	int index = m_count;
	if(!format) format = Texture::RGBA8;
	attachColour(index, Texture::create(m_width, m_height, format));
	return m_colour[index].data;
}

uint FrameBuffer::attachDepth(Texture::Format format, BufferType type) {
	if(!Texture::isDepthFormat(format)) return 0;
	if(type==TEXTURE) {
		attachDepth(Texture::create(m_width, m_height, format));
		return m_depth.data;
	}
	else if(type==RENDERBUFFER) {
		static GLuint fmt[] = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32, GL_DEPTH24_STENCIL8 };
		int glformat = fmt[ format - Texture::D16 ];
		
		m_depth.type = RENDERBUFFER;
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

void FrameBuffer::clear(const Colour& c, float depth) {
	const FrameBuffer* last = s_bound;
	if(s_bound != this) bind();
	glClearColor(c.r, c.g, c.b, c.a);
	glClearDepth(depth);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(last != this) last->bind();
}

