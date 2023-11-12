#pragma once

namespace base {
	class PNG {
		public:
		PNG() : data(0), width(0), height(0), bpp(0) {}
		PNG(const PNG&);
		~PNG() { clear(); }
		PNG(PNG&& p);
		const PNG& operator=(const PNG&);
		const PNG& operator=(PNG&&);
		
		/// Load a png image from file
		static PNG load(const char* filename);
		/// Load a png image from data held in memory
		static PNG loadEmbedded(unsigned size, const char* data);
		/// Create a new image from pixel data
		static PNG create(int width, int height, int depth, const char* data=0);
		/// Save image to file
		int save(const char* filename);
		// Clear png data
		void clear();
		
		// Image data
		char* data;
		int width, height;
		int bpp;
	};
};

