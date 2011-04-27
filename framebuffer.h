#ifndef _BASE_FRAMEBUFFER_
#define _BASE_FRAMEBUFFER_

#include "material.h"

namespace base {
class FrameBuffer {
	public:
	enum Flags { DEPTH=0x1000, COLOUR=0x2000 };

	FrameBuffer(int width, int height, int flags=COLOUR);
	~FrameBuffer();

	/** Get the framebuffer as a texture */
	const Texture& texture() const;
	operator const Texture&() const { return texture(); }

	/** Bind the framebuffer as the current a render target */
	void bind();

	/** Null framebuffer - used for unbinding */
	static FrameBuffer Screen;

	private:
	int m_width, m_height;
	int m_flags;

	Texture m_texture;

	uint m_buffer; //Frame Buffer Object
	uint m_depth;  //Depth Buffer
	
};
};

#endif

