#ifndef _BASE_TEXTURE_
#define _BASE_TEXTURE_

#define MAX_TEXTURE_UNITS 16

#include "math.h"
#include "colour.h"

namespace base {

	/** OpenGL Texture class */
	class Texture {
		public:
		enum Format { NONE,
			R8, RG8, RGB8, RGBA8,			// Standard formats
			DXT1, DXT3, DXT5,				// ST3C Compressed formats
			R32F, RG32F, RGB32F, RGBA32F,	// Float formats
			R16F, RG16F, RGB16F, RGBA16F,	// Half float formats
			R11G11G10F, R5G6B5,				// Other formats
			D16, D24, D32, D24S8 			// Depth formats
		};
		enum Filter { NEAREST, BILINEAR, TRILINEAR, ANISOTROPIC };
		enum Type   { TEX1D, TEX2D, TEX3D, CUBE, ARRAY1D, ARRAY2D };
		enum Wrapping { REPEAT, CLAMP };
		Texture();
		/** Texture creation */
		static Texture create(int width, int height, int channels, const void* data=0, bool generateMips=false);
		static Texture create(int width, int height, Format format, const void* data=0, bool generateMips=false);
		static Texture create(Type type, int width, int height, int depth, Format format, const void* data=0, bool generateMips=false);
		static Texture create(Type type, int width, int height, int depth, Format format, const void*const* data, int layers=0);

		/** Update sub image */
		int setPixels(int width, int height, Format format, const void* data, int mipLevel=0);
		int setPixels(int width, int height, Format format, const void** data, int mipmaps);
		int setPixels(int width, int height, int layer, Format format, const void* data, int miplevel=0);
		int setPixels(int width, int height, int layer, Format format, const void** data, int mipmaps);
		int setPixels(int xOffset, int yOffset, Format width, int height, int format, const void* data, int mipLevel=0);
		int setPixels(int xOffset, int yOffset, int width, int height, Format format, const void** data, int mipmaps);
		int setPixels(int xOffset, int yOffset, int zOffset, int width, int height, int depth, Format format, const void* data, int mipLevel=0);
		int setPixels(int xOffset, int yOffset, int zOffset, int width, int height, int depth, Format format, const void** data, int mipmaps);

		/** Destroy texture */
		void destroy();
		/** Bind texture */
		void bind() const;

		/** Get size */
		int width()  const { return m_width; }
		int height() const { return m_height; }
		int depth()  const { return m_depth; }
		Format getFormat() const { return m_format; }
		Type   getType() const;
		bool   isCompressed() const;

		/** Expose Opengl unit */
		unsigned unit() const { return m_unit; }

		/** Texture properties */
		void setFilter(unsigned min, unsigned mag) const;
		void setFilter(Filter filter) const;
		void setWrap(Wrapping e) const				{ setWrap(e,e,e); }
		void setWrap(Wrapping s, Wrapping t) const	{ setWrap(s,t,t); }
		void setWrap(Wrapping s, Wrapping t, Wrapping u) const;

		private:
		unsigned m_unit;	// OpenGL texture unit
		Type   m_type;	// Texture type
		Format m_format;	// Internal format
		bool m_hasMipMaps;	// Do mipmaps exist
		int m_width, m_height, m_depth;	// Size
		int generateMipMaps(int format, const void* data);
		unsigned getTarget() const;
		static unsigned convertFormat(Format);
		static unsigned getDataFormat(Format);
		static bool compressedFormat(Format);
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
		void bind() const;
		SMaterial(): ambient(0x101010), shininess(50) {}
	};
};

#endif

