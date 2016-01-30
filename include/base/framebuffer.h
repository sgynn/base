#ifndef _BASE_FRAMEBUFFER_
#define _BASE_FRAMEBUFFER_

#include "texture.h"

namespace base {
class FrameBuffer {
	public:
	enum Flags { DEPTH=0x1000, COLOUR=0x2000, BUFFER=0x4000, TEXTURE=0x8000 };

	FrameBuffer();
	FrameBuffer(int width, int height, int flags=COLOUR);
	~FrameBuffer();

	int width() const { return m_width; }
	int height() const { return m_height; }

	/** Get the framebuffer as a texture */
	const Texture& texture(int index=0) const { return m_colour[index].texture; }
	const Texture& depthTexture() const { return m_depth.texture; }
	operator const Texture&() const { return texture(); }

	/** Attach various render targets */
	uint attachColour(uint type, Texture::Format format);
	uint attachDepth(uint type, Texture::Format format);
	uint attachStencil(uint type);

	/** Bind the framebuffer as the current a render target */
	void bind() const;

	/** is this framebuffer bound? */
	bool isBound() const { return s_bound==this; }

	/** Is this a depth only buffer */
	bool isDepthOnly() const { return m_colour[0].type==0 && m_depth.type; }

	/** Is this a valid framebuffer */
	bool isValid() const;

	/** Null framebuffer - used for unbinding */
	static const FrameBuffer Screen;

	private:
	int m_width, m_height;
	int m_flags;
	uint m_buffer; //Frame Buffer Object

	//Storage objects
	struct Storage {
		uint type;
		uint data;
		Texture texture;
	};

	Storage m_colour[4];
	Storage m_depth;

	//What is bound?
	static const  FrameBuffer* s_bound;

};
};

#endif

