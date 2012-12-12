#ifndef _BASE_TEXTURE_
#define _BASE_TEXTURE_

#define MAX_TEXTURE_UNITS 16

#include "math.h"

namespace base {

	/** OpenGL Texture class */
	class Texture {
		public:
		enum Formats { ALPHA, LUMINANCE, LUMINANCE_ALPHA, RGB, RGBA };
		enum Filters { NEAREST, BILINEAR, TRILINEAR, ANISOTROPIC };
		enum Wrapping { REPEAT, CLAMP };
		Texture();
		/** Texture creation */
		static Texture create(int width, int height, int format, const char* data=0);
		static Texture create(int width, int height, int iFormat, int sFormat, int type, const void* data);
		/** Destroy texture */
		void destroy();
		/** Bind texture */
		void bind() const;
		/** Get size */
		int width() const { return m_width; }
		int height() const { return m_height; }
		int depth() const { return m_depth; }
		/** Expose Opengl unit */
		unsigned unit() const { return m_unit; }

		/** Texture properties */
		void setFilter(unsigned min, unsigned mag);
		void setFilter(int filter);
		void setWrap(unsigned e) { setWrap(e,e); }
		void setWrap(unsigned s, unsigned t);

		private:
		unsigned m_unit;
		int m_width, m_height, m_depth;
	};


	/** Simple material */
	class SMaterial {
		public:
		Colour diffuse;
		Colour ambient;
		Colour specular;
		float shininess;

		Texture texture[ MAX_TEXTURE_UNITS ];

		/** Bind material */
		void bind();
	};
};

#endif

