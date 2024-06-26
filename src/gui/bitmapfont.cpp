#include <base/gui/font.h>
#include <base/png.h>
#include <cstring>
#include <cstdio>

gui::BitmapFont::BitmapFont(const char* file, const char* characters) {
	strncpy(m_file, file, sizeof(m_file)-1);
	m_characters = characters? strdup(characters): 0;
}

bool gui::BitmapFont::build(int size) {
	base::Image image = base::PNG::load(m_file);
	if(!image) {
		printf("Failed to load bitmap font '%s'\n", m_file);
		return false;
	}
	
	unsigned char* data = image.getData();
	size_t stride = image.getBytesPerPixel();	// Pixel stride
	size_t row = image.getWidth() * stride;
	unsigned guide, clear=0;	// Colours
	memcpy(&guide, data, stride); // First pixel is the guide colour

	auto isGuide = [&](int x, int y) { return x<0 || y<0 || memcmp(&guide, data+x*stride+y*row, stride)==0; };

	// FIXME - get this data from m_characters
	createFace(size, size);
	addRange(32, 126);
	allocateGlyphs();
		

	// Loop through image
	float maxHeight = 0;
	int index = 0; // index of character being read
	Rect rect(0,0,0,0);
	unsigned glyph = 0;
	for(int y=0; y<image.getHeight(); ++y) {
		for(int x=0; x<image.getWidth(); ++x) {

			if(!isGuide(x,y) && isGuide(x-1, y) && isGuide(x, y-1)) {
				if(index==0) memcpy(&clear, data+x*stride+y*row, stride); // Get clear colour

				// Which glyph is this - TODO: UTF-8
				if(m_characters && m_characters[index]) glyph = m_characters[index];
				else if(!m_characters) glyph = index + 32;
				else { y=image.getHeight(); break; }
			
				// Find end of glyph
				rect.set(x, y, 1, 1);
				while(!isGuide(x+rect.width, y)) ++rect.width;
				while(!isGuide(x, y+rect.height)) ++rect.height;
				setGlyph(glyph, rect);

				if(rect.height > maxHeight) maxHeight = rect.height;
				x += rect.width;
				++index;
			}
		}
	}

	// Remove all guides
	unsigned end = image.getHeight() * row;
	for(unsigned i=0; i<end; i+=stride) {
		if(memcmp(&guide, data+i, stride)==0) memcpy(data+i, &guide, stride);
	}
	addImage(image.getWidth(), image.getHeight(), (unsigned char*)data);
	return true;
}

