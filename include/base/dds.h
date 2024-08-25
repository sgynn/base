#pragma once

#include <base/image.h>

namespace base {
	class DDS {
		public:
		static Image load(const char* file);
		static Image parse(const char* data, unsigned size);
		static bool  save(const Image& image, const char* filename);
	};
};

