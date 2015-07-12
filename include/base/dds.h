#ifndef _BASE_DDS_
#define _BASE_DDS_

namespace base {
	class DDS {
		public:
		/** Internal data formats */
		enum DDSFormat { INVALID, LUMINANCE, LUMINANCE_ALPHA, RGB, RGBA, DXT1, DXT3, DXT5 };
		enum DDSMode { FLAT, CUBE, VOLUME, ARRAY };

		DDS() : format(INVALID), mode(FLAT), width(0), height(0), data(0) {}
		DDS(const DDS&);
		~DDS() { clear(); }
		DDS& operator=(DDS&);
		
		/** Load the bitmap image - returns image object or NULL */
		static DDS load(const char* filename);
		/** Create a blank image */
		static DDS create(int width, int height, int depth, const char* data=0);
		/** Save image to file*/
		int save(const char* filename);
		/** Clear png data */
		void clear();

		/** Is this texture a compressed format? */
		bool isCompressed() const { return format==DXT1 || format==DXT3 || format==DXT5; }

		/** Decompress image */
		void decompress();

		/** Vertically flip image */
		void flip();

		// Image data
		DDSFormat format;
		DDSMode   mode;
		int       mipmaps;
		int       width, height, depth;
		unsigned char** data;

		private:
		void flipTexture(unsigned char* data, int width, int height, int depth);
		void decompress(const unsigned char* data, int width, int height, unsigned char* out) const;
		static int decodeR5G6B5(const unsigned char*);
		static void createTable(const unsigned char* a, const unsigned char* b, unsigned char* out);
		static void createDXT5AlphaTable(unsigned char a, unsigned char b, unsigned char* out);

	};
};

#endif
