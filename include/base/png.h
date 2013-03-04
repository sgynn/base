#ifndef _BASE_PNG_
#define _BASE_PNG_

namespace base {
	class PNG {
		public:
		PNG() : data(0), width(0), height(0), bpp(0) {}
		PNG(const PNG&);
		~PNG() { clear(); }
		const PNG& operator=(const PNG&);
		
		/** Load the bitmap image - returns image object or NULL */
		static PNG load(const char* filename);
		/** Create a blank image */
		static PNG create(int width, int height, int depth, const char* data=0);
		/** Save image to file*/
		int save(const char* filename);
		/** Clear png data */
		void clear();
		
		// Image data
		char* data;
		int width, height;
		int bpp;
	};
};

#endif
