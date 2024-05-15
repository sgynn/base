#include <base/gui/font.h>
#include <base/opengl.h>
#include <cmath>

using namespace gui;

Font::Font() {}
Font::Font(const char* source, int size) {
	SystemFont src(source);
	src.addRange(32, 126);
	addFace(src, size);
}

Font::~Font() {
}

int Font::getFontSize(int closest) const {
	if(const Face* face = selectFace(closest)) return face->size;
	return 0;
}

Point Font::getSize(const char* string, int size, int len) const {
	Point result;
	if(!string || !string[0]) return result;
	if(const Face* face = selectFace(size)) {
		float scale = (float)size / (float)face->size;
		result.y = face->height * scale;
		int line = 0;
		while(*string && len) {
			--len;
			int code = readUTF8Character(string);
			if(code == '\n') {
				if(result.x < line) result.x = line;
				result.y += face->height * scale;
				line = 0;
			}
			else {
				const Rect& r = getGlyph(*face, code);
				line += r.width * scale;
			}
		}
		if(result.x < line) result.x = line;
	}
	return result;
}

Point Font::getSize(unsigned character, int size) const {
	if(const Face* face = selectFace(size)) {
		const Rect& r = getGlyph(*face, character);
		float scale = (float)size / face->size;
		return Point(r.width * scale, r.height * scale);
	}
	return Point();
}

int Font::getLineHeight(int size) const {
	if(const Face* face = selectFace(size)) {
		return face->height * (float)size / (float)face->size;
	}
	return 0;
}

const Font::Face* Font::selectFace(int size) const {
	int diff = 9999;
	const Face* face = nullptr;
	for(const Face& i: m_faces) {
		int d = abs(size - i.size);
		if(d < diff) { diff=d; face = &i; }
	}
	return face;
}

const Rect& Font::getGlyph(const Face& face, unsigned code) const {
	for(const GlyphRange& range: face.groups) {
		if(code < range.start) break;
		if(code < range.start + range.glyphs.size()) return range.glyphs[code - range.start];
	}
	static const Rect nope;
	return nope;
}



// ====================================================================================== //

bool FontLoader::build(Font* parent, int size) {
	m_font = parent;
	m_face = nullptr;
	return build(size);
}

void FontLoader::addRange(int start, int end) {
	// ToDo: Detect overlaps
	m_glyphs.push_back({start, end});
}

void FontLoader::createFace(int size, int height) {
	m_font->m_faces.push_back({size, height});
	m_face = &m_font->m_faces.back();
}

Point FontLoader::selectImageSize(int size, int count) const {
	unsigned pitch = (int)ceil(sqrt(count)) * size;
	// Round up to next power of 2
	--pitch;
	pitch |= pitch>>1;
	pitch |= pitch>>2;
	pitch |= pitch>>4;
	pitch |= pitch>>8;
	pitch |= pitch>>16;
	++pitch;
	return Point(pitch, pitch);
}

void FontLoader::addImage(int w, int h, const unsigned char* pixels) {
	if(m_font->m_texture==0) {
		glGenTextures(1, &m_font->m_texture);
		glBindTexture(GL_TEXTURE_2D, m_font->m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		m_font->m_textureSize.set(w, h);
	}
	else {
		// Resize image
		// copy old data
		// set new data
		// move glyph rects
	}
}

int FontLoader::countGlyphs() const {
	int count = 0;
	for(const Range& range : m_glyphs) count += range.end - range.start + 1;
	return count;
}

void FontLoader::allocateGlyphs() {
	for(const Range& r: m_glyphs) {
		m_face->groups.push_back({(unsigned)r.start});
		m_face->groups.back().glyphs.resize(r.end - r.start + 1, Rect()); 
	}
}

bool FontLoader::setGlyph(unsigned code, const Rect& r) {
	for(Font::GlyphRange& range: m_face->groups) {
		if(code < range.start) break;
		if(code < range.start + range.glyphs.size()) {
			range.glyphs[code - range.start] = r;
			return true;
		}
	}
	return false;
}

