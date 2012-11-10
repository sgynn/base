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
		void clear();
		void flip();
		
		// Image data
		char* data;
		int width, height;
		int bpp;
	};
};

#endif
