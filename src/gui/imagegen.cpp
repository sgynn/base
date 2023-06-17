#include <base/gui/renderer.h>
#include <base/png.h>
#include <base/opengl.h>
#include <assert.h>

using namespace gui;

struct GeneratedImagePixels { int width=0, height=0; uint* data=0; };

inline int max(int a, int b) { return a>b? a: b; }
inline int min(int a, int b) { return a<b? a: b; }

inline uint colourShift(uint colour, int amount) {
	for(int i=0; i<24; i+=8) {
		int value = ((colour>>i) & 0xff) + amount;
		value = value<0? 0: value>255? 255: value;
		colour = (colour&~(0xff<<i)) | (value<<i);
	}
	return colour;
}
inline uint desaturate(uint colour) {
	int value = (colour&0xff) + ((colour>>8)&0xff)*2 + ((colour>>16)&0xff);
	value /= 4;
	if(value > 0xff) value = 0xff;
	return (colour&0xff000000) | value | (value<<8) | (value<<16);
}


static GeneratedImagePixels generateSkinImage(const ImageGenerator& info) {
	int cornerSize = max(1, info.radius);
	Point boxSize(info.core.x + 2*cornerSize, info.core.y + 2*cornerSize);

	// Map region: 0:out, 1:back, 2=border
	char* mask = new char[boxSize.x * boxSize.y];
	memset(mask, 0, boxSize.x * boxSize.y);

	auto set = [&](int corner, int x, int y, char v) {
		if(corner & 1) x = boxSize.x - x - 1;
		if(corner & 2) y = boxSize.y - y - 1;
		mask[x+boxSize.x*y] = v;
	};

	// Draw lines
	int width = max(1, min(info.lineWidth, info.radius));
	for(int corner=0; corner<4; ++corner) {
		switch(info.corner[corner]) {
		case ImageGenerator::Square:
			set(corner, 0, 0, 2);
			for(int i=1; i<cornerSize; ++i) {
				for(int j=0; j<width; ++j) {
					set(corner, i, j, 2);
					set(corner, j, i, 2);
				}
			}
			break;
		case ImageGenerator::Chamfered:
			for(int j=0; j<width; ++j) {
				for(int i=0; i<cornerSize-j; ++i) {
					set(corner, i+j, cornerSize-i, 2);
				}
			}
			break;
		case ImageGenerator::Rounded:
			// Midpoint algorithm ?
			for(int j=0; j<width; ++j) {
				int x=cornerSize-j, y=0, p=1-cornerSize;
				while(x>y) {
					++y;
					if(p<=0) p += 2*y+1;
					else {
						--x;
						p += 2*y - 2*x + 1;
					}
					set(corner, cornerSize-x, cornerSize-y, 2);
					set(corner, cornerSize-y, cornerSize-x, 2);
				}
			}
		}
	}

	// Fix holes created by midpoint algorithm
	if(width>1) {
		int e = (boxSize.x-1)*boxSize.y-1;
		for(int i=boxSize.x+1; i<e; ++i) {
			if(mask[i]==0 && mask[i+1]==2 && mask[i-1]==2 && mask[i+boxSize.x]==2 && mask[i-boxSize.x]==2) {
				mask[i] = 2;
			}
		}
	}


	// Sides
	for(int j=0; j<width; ++j) {
		for(int x=cornerSize; x<cornerSize+info.core.x; ++x) {
			mask[x+boxSize.x*j] = 2;
			mask[x+boxSize.x*(boxSize.y-1-j)] = 2;
		}
		for(int y=cornerSize; y<cornerSize+info.core.y; ++y) {
			mask[y*boxSize.x+j] = 2;
			mask[y*boxSize.x + boxSize.x-1 - j] = 2;
		}
	}

	// Fill centre (floodfill)
	std::vector<int> open;
	open.push_back(boxSize.x*(boxSize.y/2) + boxSize.x/2);
	for(size_t i=0; i<open.size(); ++i) {
		int k = open[i];
		assert(k>=0 && k<boxSize.x*boxSize.y); // Floodfill error
		if(mask[k] == 0) {
			mask[k] = 1;
			open.push_back(k+1);
			open.push_back(k-1);
			open.push_back(k+boxSize.x);
			open.push_back(k-boxSize.x);
			
		}
	}

	// ------------- //
	
	// Create image buffer
	int stateCount = 0;
	Point imageSize = boxSize;
	switch(info.type) {
	case ImageGenerator::Single: stateCount=1; break;
	case ImageGenerator::Four:   stateCount=4; imageSize.x *= 2; imageSize.y *= 2; break;
	case ImageGenerator::Eight:  stateCount=8; imageSize.x *= 2; imageSize.y *= 4; break;
	default: return {0,0,0}; // Error
	}

	GeneratedImagePixels image { imageSize.x, imageSize.y };
	image.data = new uint[imageSize.x * imageSize.y];
	memset(image.data, 0, imageSize.x * imageSize.y * 4);

	// copy mask to image and colourise //
	for(int state=0; state<stateCount; ++state) {
		ImageGenerator::ColourPair c = info.colours[state];
		uint* start = image.data + (state&1) * boxSize.x + (state/2)*boxSize.y*imageSize.x;
		for(int y=0; y<boxSize.y; ++y) {
			for(int x=0; x<boxSize.x; ++x) {
				uint& out = start[x + y * imageSize.x];
				char mode = mask[x+y*boxSize.x];
				// Convert to colour
				if(mode==1) out = c.back;
				else if(mode==2) out = c.line;
				out = (out&0xff00ff00) | ((out>>16)&0xff) | ((out<<16)&0xff0000); // RGB->BGR
			}
		}
	}
	
	delete [] mask;
	return image;
}

static GeneratedImagePixels generateGlyphImage(int size) {
	if(size < 4) return {};

	const int glyphCount = 6;	// < > ^ v o x
	Point glyph[glyphCount];
	for(int i=0; i<glyphCount; ++i)  glyph[i].set(i%3 * size, i/3 * size);
	GeneratedImagePixels image { size*3, size*2 };
	image.data = new uint[image.width * image.height];
	memset(image.data, 0, image.width * image.height * 4);

	auto paint = [&](int x, int y, uint c) { image.data[x+y*image.width] = c; };

	// Arrows
	int w = size / 2 - 1;
	int b = w / 2 + 1;
	Point ctr[glyphCount];
	for(int i=0; i<glyphCount; ++i) {
		ctr[i].x = glyph[i].x + size/2;
		ctr[i].y = glyph[i].y + size/2;
	}
	for(int l=0; l<w; ++l) {
		for(int w=0; w<l; ++w) {
			paint(ctr[0].x+w, ctr[0].y+l-b, 0xffffffff);
			paint(ctr[0].x-w, ctr[0].y+l-b, 0xffffffff);

			paint(ctr[1].x+w, ctr[1].y-l+b, 0xffffffff);
			paint(ctr[1].x-w, ctr[1].y-l+b, 0xffffffff);

			paint(ctr[2].x+l-b, ctr[2].y+w, 0xffffffff);
			paint(ctr[2].x+l-b, ctr[2].y-w, 0xffffffff);

			paint(ctr[3].x-l+b, ctr[3].y+w, 0xffffffff);
			paint(ctr[3].x-l+b, ctr[3].y-w, 0xffffffff);
		}
	}
	// Circle
	int radius = max(1, size/2 - 3);
	int x=radius, y=0, p=1-radius;
	paint(ctr[4].x+x, ctr[4].y, 0xffffffff);
	paint(ctr[4].x-x, ctr[4].y, 0xffffffff);
	paint(ctr[4].x, ctr[4].y+x, 0xffffffff);
	paint(ctr[4].x, ctr[4].y-x, 0xffffffff);
	while(x>y) {
		++y;
		if(p<=0) p += 2*y+1;
		else {
			--x;
			p += 2*y - 2*x + 1;
		}
		paint(ctr[4].x+x, ctr[4].y+y, 0xffffffff);
		paint(ctr[4].x-x, ctr[4].y+y, 0xffffffff);
		paint(ctr[4].x+x, ctr[4].y-y, 0xffffffff);
		paint(ctr[4].x-x, ctr[4].y-y, 0xffffffff);
		paint(ctr[4].x+y, ctr[4].y+x, 0xffffffff);
		paint(ctr[4].x-y, ctr[4].y+x, 0xffffffff);
		paint(ctr[4].x+y, ctr[4].y-x, 0xffffffff);
		paint(ctr[4].x-y, ctr[4].y-x, 0xffffffff);
	}
	std::vector<int> fill;
	fill.push_back(ctr[4].x+ctr[4].y*image.width);
	for(size_t i=0; i<fill.size(); ++i) {
		int k = fill[i];
		if(image.data[k]==0) {
			image.data[k] = 0xffffffff;
			fill.push_back(k+1);
			fill.push_back(k-1);
			fill.push_back(k+image.width);
			fill.push_back(k-image.width);
		}
	}


	// Cross
	int s = max(2, size / 2 - 3);
	int t = max(1, size / 6);
	for(int i=0; i<s; ++i) {
		for(int j=0; j<t; ++j) {
			paint(ctr[5].x+i-j, ctr[5].y+i, 0xffffffff);
			paint(ctr[5].x+i, ctr[5].y+i-j, 0xffffffff);
			paint(ctr[5].x-i+j, ctr[5].y+i, 0xffffffff);
			paint(ctr[5].x-i, ctr[5].y+i-j, 0xffffffff);
			paint(ctr[5].x+i-j, ctr[5].y-i, 0xffffffff);
			paint(ctr[5].x+i, ctr[5].y-i+j, 0xffffffff);
			paint(ctr[5].x-i+j, ctr[5].y-i, 0xffffffff);
			paint(ctr[5].x-i, ctr[5].y-i+j, 0xffffffff);
		}
	}

	return image;
}

int Renderer::generateImage(const char* name, const ImageGenerator& data) {
	GeneratedImagePixels image;
	switch(data.type) {
	case ImageGenerator::Glyphs: image = generateGlyphImage(data.radius); break;
	case ImageGenerator::Single: image = generateSkinImage(data); break;
	case ImageGenerator::Four:   image = generateSkinImage(data); break;
	case ImageGenerator::Eight:  image = generateSkinImage(data); break;
	}

	// Store image
	if(!image.data) return -1;
	int exists = getImage(name);
	if(exists < 0) return addImage(name, image.width, image.height, 4, image.data);
	else replaceImage(exists, image.width, image.height, 4, image.data);
	delete [] image.data;
	return exists;
}


Skin* Renderer::createDefaultSkin(unsigned line, unsigned back) {
	ImageGenerator gen;
	gen.type = ImageGenerator::Four;
	gen.colours[0].line = line;
	gen.colours[0].back = back;
	gen.colours[1].line = colourShift(line, 40);
	gen.colours[1].back = back;
	gen.colours[2].line = colourShift(line, -30);
	gen.colours[2].back = colourShift(back, -30);
	gen.colours[3].line = colourShift(line, -60);
	gen.colours[3].back = colourShift(back, -30);
	int image = generateImage("default", gen);

	// Build skin
	Skin* s = new Skin;
	s->setImage(image);
	s->setState(Skin::NORMAL,   Rect(0,0,4,4), 2);
	s->setState(Skin::OVER,     Rect(4,0,4,4), 2);
	s->setState(Skin::PRESSED,  Rect(0,4,4,4), 2);
	s->setState(Skin::DISABLED, Rect(4,4,4,4), 2);
	return s;
}



