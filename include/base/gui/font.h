// New font class thet can tender TTF fonts

#pragma once

#include <base/point.h>
#include <vector>

namespace gui {

class Font {
	public:
	Font(const char* name, int size);
	Font(const char* name, const char* characters=0);
	~Font();

	/** Render a string to the screen */
	//const Point& print(int x, int y, const char* text) const { return print(x,y,m_size,text); }
	//const Point& print(int x, int y, int size, const char* text) const;
	/** Render a formatted string to the screen */
	//const Point& printf(int x, int y, const char* format, ... ) const;
	//const Point& printf(int x, int y, int size, const char* format, ... ) const;
	/** Get the size in texels of a string */
	Point getSize(const char* c, int size=0, int len=-1) const;
	/** Get the size of a character */
	Point getSize(const char c, int size=0) const;

	float getScale(int size) const { return size? (float)size/m_size: 1.f; }
	const Rect& getGlyph(char c) const { return m_glyphs[c>0?c:0]; }
	int getLineHeight(int size=0) const { return (int)(m_glyphHeight * getScale(size) + 0.5f); }

	unsigned getTexture() const { return m_texture; }
	const Point getTextureSize() const { return Point(m_w, m_h); }


	private:
	/** Generate font image from system font */
	unsigned char* build(const char* name, int size, int width, int height);
	/** Extract glyph data for a bitmap font */
	int findGlyphs(char* bmp, int w, int h, int bpp, const char* characters=0);

	private:
	friend class FontLoader;
	Font();
	unsigned m_texture;	// OpenGL texture unit
	int m_w, m_h;		// Texture size
	int m_size;			// Rendered font size
	int m_glyphHeight; 	// Everything has the same height
	std::vector<Rect> m_glyphs; // Glyph texture coordinates
};

struct GlyphRange { unsigned min, max; };
using GlyphRangeVector = std::vector<GlyphRange>;

class FontLoader {
	protected:
	static Font* createFontObject(int size, int height);
	static void  setFontSize(Font* font, int size, int height);
	static void  setFontImage(Font* font, int w, int h, void* data);
	static Point selectImageSize(int fontSize, int glyphCount);
	static void  setGlyph(Font* font, unsigned code, const Rect& rect);
};

class SystemFont : public FontLoader {
	public:
	static Font* load(const char* name, int size, const GlyphRangeVector& characters={{32,126}});
};

class FreeTypeFont : public FontLoader {
	public:
	static Font* load(const char* name, int size, const GlyphRangeVector& characters={{32,126}});
};

class BitmapFont : public FontLoader {
	public:
	static Font* load(const char* name, const char* characters=0);
};





}

