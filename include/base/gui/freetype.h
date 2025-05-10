#pragma once

#include <base/gui/font.h>

namespace gui {
class FreeTypeFont : public FontLoader {
	public:
	FreeTypeFont(const char* file);
	bool build(int size) override;
	protected:
	char m_file[128];
};

// Fun hack to make freetype linkable just by including this file
class FreeTypeAutoRegister {
	public:
	FreeTypeAutoRegister() {
		FontLoader::getFreetypeLoader() = [](const char* f)->FontLoader*{return new FreeTypeFont(f); };
	}
};
static FreeTypeAutoRegister freeTypeAutoRegister;
}

