#pragma once

#include "texture.h"
#include "point.h"

namespace base {
class FrameBuffer {
	public:
	enum BufferType { NONE, TEXTURE, RENDERBUFFER };
	FrameBuffer(int width, int height);
	template<class...B> FrameBuffer(int width, int height, B...buffers);
	~FrameBuffer();

	int width() const { return m_width; }
	int height() const { return m_height; }
	Point getSize() const { return Point(m_width, m_height); }

	/** Get the framebuffer as a texture */
	const Texture& texture(int index=0) const { return m_colour[index].texture; }
	const Texture& depthTexture() const { return m_depth.texture; }
	operator const Texture&() const { return texture(); }

	/** Attach various render targets */
	uint attachColour(Texture::Format format);
	void attachColour(uint index, const Texture& texture);
	void attachDepth(const Texture& texture);
	uint attachDepth(Texture::Format format, BufferType type=TEXTURE);
	uint attachStencil(uint type);

	/** Bind the framebuffer as the current a render target */
	bool bind() const;
	bool bind(const Rect& r) const;

	/** is this framebuffer bound? */
	bool isBound() const { return s_bound==this; }

	/** Is this a depth only buffer */
	bool isDepthOnly() const { return m_count==0; }

	/** Is this a valid framebuffer */
	bool isValid() const;

	/** Null framebuffer - used for unbinding */
	static const FrameBuffer& Screen;
	static const FrameBuffer& current() { return *s_bound; }
	static void setScreenSize(int w, int h);

	private:
	FrameBuffer();
	void attachTexture(Texture::Format f);
	template<class...B> void attachTexture(Texture::Format fmt, B...more) {
		attachTexture(fmt);
		attachTexture(more...);
	}

	private:
	static FrameBuffer s_screen;
	int m_width = 0;
	int m_height = 0;
	uint m_buffer = 0; //Frame Buffer Object

	//Storage objects
	struct Storage {
		BufferType type = NONE;
		uint data = 0;
		Texture texture;
	};

	Storage m_colour[4];	// Colour buffers
	Storage m_depth;		// Depth buffer
	uint    m_count = 0;	// Number of attached colour buffers

	//What is bound?
	static const  FrameBuffer* s_bound;

};
}

template<class...B>
base::FrameBuffer::FrameBuffer(int width, int height, B...buffers) : FrameBuffer(width, height) {
	if(m_buffer) {
		const FrameBuffer* previous = s_bound;
		bind();
		attachTexture(buffers...); 
		previous->bind();
	}
}

