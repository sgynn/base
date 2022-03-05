#include <base/gui/font.h>
#include <base/png.h>
#include <cstring>

gui::Font* gui::BitmapFont::load(const char* file, const char* characters) {
	base::PNG png = base::PNG::load(file);
	if(!png.data) return 0;
	
	Font* font = createFontObject(0,0);

	char* data = png.data;
	size_t stride = png.bpp/8;	// Pixel stride
	size_t row = png.width*stride;
	unsigned guide, clear=0;	// Colours
	memcpy(&guide, data, stride); // First pixel is the guide colour

	auto isGuide = [&](int x, int y) { return x<0 || y<0 || memcmp(&guide, data+x*stride+y*row, stride)==0; };

	// Loop through image
	float maxHeight = 0;
	int index = 0; // index of character being read
	Rect rect(0,0,0,0);
	unsigned glyph = 0;
	for(int y=0; y<png.height; ++y) {
		for(int x=0; x<png.width; ++x) {

			if(!isGuide(x,y) && isGuide(x-1, y) && isGuide(x, y-1)) {
				if(index==0) memcpy(&clear, data+x*stride+y*row, stride); // Get clear colour

				// Which glyph is this
				if(characters && characters[index]) glyph = characters[index];
				else if(!characters) glyph = index + 32;
				else { y=png.height; break; }
			
				// Find end of glyph
				rect.set(x, y, 1, 1);
				while(!isGuide(x+rect.width, y)) ++rect.width;
				while(!isGuide(x, y+rect.height)) ++rect.height;
				setGlyph(font, glyph, rect);

				if(rect.height > maxHeight) maxHeight = rect.height;
				x += rect.width;
				++index;
			}
		}
	}

	// Remove all guides
	unsigned end = png.height * row;
	for(unsigned i=0; i<end; i+=stride) {
		if(memcmp(&guide, data+i, stride)==0) memcpy(data+i, &guide, stride);
	}
	setFontImage(font, png.width, png.height, data);
	setFontSize(font, maxHeight, maxHeight);
	return font;
}

