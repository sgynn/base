#pragma once

namespace base {
	class Image {
		public:
		enum Mode { FLAT, CUBE, VOLUME, ARRAY };
		enum Format { INVALID, R8, RG8, RGB8, RGBA8, BC1, BC3, BC4, BC5, R16 };
		typedef unsigned char byte;

		Image() = default;
		Image(const Image&);
		Image(Image&&);
		Image(Format, int w, int h, byte* data);
		Image(Mode, Format, int w, int h, int d, byte* data);
		Image(Mode, Format, int w, int h, int d, int faces, byte** data);
		virtual ~Image() { for(byte i=0; i<m_faces;++i) delete [] m_data[i]; delete [] m_data; }
		Image& operator=(const Image&);
		Image& operator=(Image&&);

		Mode getMode { return m_mode; }
		Format getFormat() { return m_format; }
		int getFaces() const { return m_faces; }
		int getWidth() const { return m_width; }
		int getHeight() const { return m_height; }
		int getDepth() const { return m_depth; }
		const byte* getData(int face=0) const { return face<faces?m_data[face]: nullptr; }
		bool isCompressed() const { return m_format>=BC1 && m_format<=BC5; }
		void decompress();
		protected:
		Mode m_mode = FLAT;
		Format m_format = INVALID;
		int m_width = 0;
		int m_height = 0;
		int m_depth = 0;
		byte m_faces = 0;			// Number of individual images
		byte** m_data = nullptr;	// Raw data
	};
}

