#ifndef _BASE_FRAMEBUFFER_
#define _BASE_FRAMEBUFFER_

#include "material.h"

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
	uint attachColour(uint type, uint format=0, bool isFloat=false);
	uint attachDepth(uint type, uint depth=32);
	uint attachStencil(uint type);

	/** Bind the framebuffer as the current a render target */
	void bind() const;

	/** is this framebuffer bound? */
	bool isBound() const { return s_bound==this; }

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

