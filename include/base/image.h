#pragma once

namespace base {
	class Image {
		public:
		enum Mode { FLAT, CUBE, VOLUME, ARRAY };
		enum Format { INVALID, R8, RG8, RGB8, RGBA8, BC1, BC2, BC3, BC4, BC5, R16, R16F, R32F };
		enum PixelType { UNKNOWN, BYTE, WORD, HALFFLOAT, FLOAT };
		typedef unsigned char byte;

		~Image();
		Image() = default;
		Image(const Image&) = delete;
		Image(Image&&) noexcept;
		Image(Format, int w, int h, byte* data);
		Image(Mode, Format, int w, int h, int d, int mips, byte** data);
		Image& operator=(const Image&) = delete;
		Image& operator=(Image&&) noexcept;
		operator bool() const { return m_data; }

		Mode getMode() const { return m_mode; }
		Format getFormat() const { return m_format; }
		int getMips() const { return m_mips; }
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }
		int getDepth() const { return m_depth; }
		int getChannels() const      { return getChannels(m_format); }
		int getBytesPerPixel() const { return getBytesPerPixel(m_format); }
		int getFaces() const { return m_data? m_mode==CUBE? 6: 1: 0; }
		bool isCompressed() const { return isCompressed(m_format); }
		byte* getData(int mip=0) { return getData(0, mip); }
		byte* getData(int face, int mip) { return face<getFaces() && mip<getMips()? m_data[face + mip*getFaces()]: nullptr; }
		const byte* getData(int mip=0) const { return getData(0, mip); }
		const byte* getData(int face, int mip) const { return face<getFaces() && mip<getMips()? m_data[face + mip*getFaces()]: nullptr; }
		const byte*const* getDataPtr() const { return m_data; }

		// Gets the data and clears the internal pointer. Used to avoid making a copy where we don't care about the original
		byte* stealData(int mip=0) { return stealData(0, mip); }
		byte* stealData(int face, int mip);

		public:
		static bool isCompressed(Format f)    { return f>=BC1 && f<=BC5; }
		static int getChannels(Format f)      { constexpr const int c[]={0,1,2,3,4,3,4,4,1,2,1,1,1}; return c[f]; }
		static int getBytesPerPixel(Format f) { constexpr const int b[]={0,1,2,3,4,0,0,0,0,0,2,2,4}; return b[f]; }
		static PixelType getPixelType(Format f);

		void flip();
		Image convert(Format, const char* swizzle=nullptr) const;
		private:
		void flipSurface(byte* data, int w, int h, int d);
		byte* convertSurface(const byte* data, int w, int h, int d, Format to, const char* swizzle) const;
		protected:
		Mode m_mode = FLAT;
		Format m_format = INVALID;
		int m_width = 0;
		int m_height = 0;
		int m_depth = 0;
		byte m_mips = 0;			// Number of mipmaps, includes primary
		byte** m_data = nullptr;	// Raw data
	};
}

