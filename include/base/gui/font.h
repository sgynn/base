#pragma once

#include <base/point.h>
#include <vector>

namespace gui {
class Font {
	public:
	struct Range { unsigned start, end; };
	using GlyphRangeVector = std::vector<Range>;

	public:
	Font();
	Font(const char* source, int size=24);
	~Font();
	
	// Add a new face to this font
	template<class Loader> bool addFace(Loader& loader, int size);

	int getFontSize(int closest=0) const;	// get base font size
	Point getSize(const char* string, int size, int len=-1) const;
	Point getSize(unsigned character, int size) const;
	int getLineHeight(int size) const;

	template<class VxFunc, class IxFunc>
	int buildVertexArray(const char* string, int len, float size, Point& pos, const Rect& clip, const VxFunc& vfunc, const IxFunc& ifunc) const;
	unsigned getTexture() const { return m_texture; }

	static int readUTF8Character(const char*& text);

	private:
	friend class FontLoader;
	
	struct GlyphRange {
		size_t start;
		std::vector<Rect> glyphs;
	};
	struct Face {
		int size;
		int height;
		std::vector<GlyphRange> groups;
	};
	std::vector<Face> m_faces;
	unsigned m_texture = 0;
	Point m_textureSize;

	const Face* selectFace(int size) const;
	const Rect& getGlyph(const Face& face, unsigned character) const;
};


class FontLoader {
	public:
	FontLoader() {}
	virtual ~FontLoader() {}
	virtual bool build(int size) = 0;
	void addRange(int start, int end);
	bool build(Font* parent, int size);
	int countGlyphs() const;
	protected:
	void createFace(int size, int lineHeight);
	void addImage(int w, int h, const unsigned char* pixels);
	bool setGlyph(unsigned code, const Rect& rect);
	void allocateGlyphs();
	Point selectImageSize(int size, int count) const;
	protected:
	struct Range { int start, end; };
	std::vector<Range> m_glyphs;
	private:
	Font::Face* m_face = 0;
	Font* m_font = 0;
};

class SystemFont : public FontLoader {
	public:
	SystemFont(const char* name);
	bool build(int size) override;
	protected:
	char m_name[32];
};

class FreeTypeFont : public FontLoader {
	public:
	FreeTypeFont(const char* file);
	bool build(int size) override;
	protected:
	char m_file[128];
};

class BitmapFont : public FontLoader {
	public:
	BitmapFont(const char* file, const char* characters);
	bool build(int size) override;
	protected:
	char m_file[128];
	char* m_characters;
};

}

inline int gui::Font::readUTF8Character(const char*& text) {
	const char* c = text;
	if((*c&0x80)==0)    { text+=1; return c[0]; }
	if((*c&0xe0)==0xc0) { text+=2; return (c[0]&0x1f<<6)  | (c[1]&0x3f); }
	if((*c&0xf0)==0xe0) { text+=3; return (c[0]&0x0f<<12) | (c[1]&0x3f<<6)  | (c[2]&0x3f); }
	if((*c&0xf8)==0xf0) { text+=4; return (c[0]&0x07<<18) | (c[1]&0x3f<<12) | (c[2]&0x3f<<6) | (c[3]&0x3f); }
	++text;
	return 0; // Error: Invalid utf-8 character
}

template<class Loader> bool gui::Font::addFace(Loader& loader, int size) {
	return static_cast<FontLoader*>(&loader)->build(this, size);
}

template<class VxFunc, class IxFunc>
int gui::Font::buildVertexArray(const char* text, int len, float size, Point& pos, const Rect& rect, const VxFunc& vfunc, const IxFunc& ifunc) const {
	const Face* face = selectFace(size);
	if(!face || !text) return 0;

	float scale = (float)size / face->size;
	float ix = 1.f / m_textureSize.x;
	float iy = 1.f / m_textureSize.y;
	Rect dst = pos;
	unsigned k = 0;
	unsigned code = 0;
	if(len<0) len = 1<<24;
	while(*text && len) {
		--len;
		code = readUTF8Character(text);
		if(code=='\n') {
			dst.x = pos.x;
			dst.y += face->height * scale;
			if(dst.y >= rect.bottom()) break;
		}
		else {
			const Rect& src = getGlyph(*face, code);
			if(src.width) {
				dst.width = src.width * scale;
				dst.height = src.height * scale;
				if(rect.intersects(dst)) {
					vfunc(dst.x,       dst.y,        src.x*ix,       src.y*iy);
					vfunc(dst.right(), dst.y,        src.right()*ix, src.y*iy);
					vfunc(dst.x,       dst.bottom(), src.x*ix,       src.bottom()*iy);
					vfunc(dst.right(), dst.bottom(), src.right()*ix, src.bottom()*iy);
					ifunc(k);
					ifunc(k+2);
					ifunc(k+1);
					ifunc(k+1);
					ifunc(k+2);
					ifunc(k+3);
					k+=4;
				}
				dst.x += dst.width;
			}
		}
	}
	pos = dst.position();
	return k/4;
}

