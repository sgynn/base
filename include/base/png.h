#ifndef _BASE_PNG_
#define _BASE_PNG_

namespace base {
	class PNG {
		public:
		PNG() : data(0), width(0), height(0) {}
		~PNG() { clear(); }
		
		/** Load the bitmap image - returns image object or NULL */
		bool load(const char* filename);
		void clear();
		void flip();
		
		// Image data
		char* data;
		int width, height;
		int bpp;
		private:
		int iFormat;
	};
};

#endif
