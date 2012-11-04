#ifndef _BASE_FONT_
#define _BASE_FONT_

//A class to add text in SDL appslications. Uses sprite fonts.
namespace base {
class Font {
	public:
	Font(const char* filename);
	~Font();
	/** Render a string to the screen (using whatever the current projection is, to whatever render target...) */
	int print(int x, int y, const char* text) const { return print(x,y,1,text); }
	int print(int x, int y, float scale, const char* text) const;
	/** Render a formatted string to the screen */
	int printf(int x, int y, const char* format, ... ) const;
	int printf(int x, int y, float scale, const char* format, ... ) const;
	/** Get the width in texels of a string */
	int textWidth(const char* text) const;
	/** Get the height in texels of a string. */
	int textHeight(const char* text) const;
	private:
	unsigned int loadTexture(const char* file);
	unsigned int getGlyphs(const void* data, int w, int h, int bpp=32);
	unsigned int m_tex;
	unsigned int m_w, m_h;
	struct Glyph { int x, y, w, h; };
	Glyph m_glyph[128];
};
};

#endif
