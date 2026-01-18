#pragma once

#include <base/image.h>

namespace base {
	class PNG {
		public:
		static Image load(const char* file);
		static Image parse(const char* data, unsigned size, bool singlePixel=false);
		static bool save(const Image& image, const char* file);
	};
}

