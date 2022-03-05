#include <base/gui/font.h>
#include <base/opengl.h>
#include <base/texture.h>
#include <base/png.h>
#include <base/point.h>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#ifdef LINUX
#include <X11/Xlib.h>
#endif

#ifdef WIN32
#include <base/game.h>
#include <base/window.h>
#endif

#ifdef FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#endif

#if !defined(LINUX) && !defined(WIN32) && !defined(FREETYPE)
#error "No font system defined"
#endif

using namespace gui;

gui::Font* FontLoader::createFontObject(int size, int height) {
	Font* font = new Font();
	font->m_texture = 0;
	font->m_w = 0;
	font->m_h = 0;
	font->m_size = size;
	font->m_glyphHeight = height;
	return font;
}

void FontLoader::setFontSize(Font* font, int size, int height) {
	font->m_size = size;
	font->m_glyphHeight = height;
}

Point FontLoader::selectImageSize(int size, int count) {
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

void FontLoader::setFontImage(Font* font, int w, int h, void* data) {
	font->m_w = w;
	font->m_h = h;
	base::Texture t = base::Texture::create(w, h, 4, data);
	font->m_texture = t.unit();
}


void FontLoader::setGlyph(Font* font, unsigned code, const Rect& r) {
	while(font->m_glyphs.size()<=code) font->m_glyphs.emplace_back(0,0,0,0);
	font->m_glyphs[code] = r;
}

// ======================================================================================================== //

gui::Font::Font() : m_w(0), m_h(0), m_size(0), m_glyphHeight(0) {
}

gui::Font::Font(const char* name, int size) : m_size(size) {
	// Guess a reasonable texture size
	m_w = 64;
	while(m_w < size * 12) m_w*=2;
	m_h = m_w/2;
	m_glyphHeight = 0;
	// Create font
	unsigned char* data = build(name, size, m_w, m_h);
	base::Texture t = base::Texture::create(m_w, m_h, 4, data);
	m_texture = t.unit();
	delete [] data;
}
gui::Font::Font(const char* file, const char* characters) : m_w(0), m_h(0), m_size(0) {
	// Load image
	m_glyphHeight = 0;
	base::PNG png = base::PNG::load(file);
	if(png.data && findGlyphs(png.data, png.width, png.height, png.bpp/8, characters)>0) {
		base::Texture t = base::Texture::create(png.width, png.height, png.bpp/8, png.data);
		m_texture = t.unit();
	} else {
		::printf("Failed to load image font %s\n", file);
		m_w = m_h = 0;
	}
}

gui::Font::~Font() {
	if(m_w>0) glDeleteTextures(1, &m_texture);
}


// ======================================================================================================== //

Point gui::Font::getSize(char c, int s) const {
	if(s<=0) s = m_size;
	const Point& g = m_glyphs[ (int)c ].size();
	return Point( g.x * s / m_size, g.y * s / m_size );
}


Point gui::Font::getSize(const char* text, int size, int len) const {
	Point s;
	int cx=0, cy=0;
	++len;
	if(size<=0) size = m_size;
	float scale = (float) size / m_size;
	for(const char* c=text; *c && --len; c++) {
		if(*c=='\n') cx = 0, cy = s.y;
		else {
			const Point& g = m_glyphs[ (int)*c ].size();
			cx += g.x * scale;
			if(cx > s.x) s.x = cx;
			if(cy + g.y * scale > s.y) s.y = cy + g.y * scale;
		}
	}
	return s;
}

// ======================================================================================================== //

/*

const Point& gui::Font::printf(int x, int y, const char* format, ... ) const {
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	va_end (args);
	return print(x,y, buffer);
}
const Point& gui::Font::printf(int x, int y, int size, const char* format, ... ) const {
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	va_end (args);
	return print(x,y,size, buffer);
}
*/

// ================================##===============================##==================================== //

/*
inline void addVertexToArray(float* v, float x, float y, float tx, float ty) {
	v[0] = x; v[1] = y;; v[2] = tx; v[3] = ty;
}

const Point& gui::Font::print(int x, int y, int size, const char* text) const {
	static Point p;
	if(!text || !text[0] || m_w==0) return p.x=x, p.y=y, p;
	int l = strlen(text);
	float scale = (float)size / m_size;
	float sw = 1.0f / m_w;
	float sh = 1.0f / m_h;
	
	// Draw in batches of up to 64 characters
	static float vx[1024];
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glVertexPointer(2, GL_FLOAT, 4*sizeof(float), vx);
	glTexCoordPointer(2, GL_FLOAT, 4*sizeof(float), vx+2);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	int cx=x, cy=y, line=0, vi=0;
	for(int i=0; i<l; ++i) {
		const Rect& g = m_glyphs[ (int)text[i] ];
		if(g.height>line) line=g.height;
		if(g.width>0) {
			addVertexToArray(vx+vi,    cx,               cy,                g.x*sw,           g.y*sh);
			addVertexToArray(vx+vi+4,  cx,               cy+g.height*scale, g.x*sw,           (g.y+g.height)*sh);
			addVertexToArray(vx+vi+8,  cx+g.width*scale, cy+g.height*scale, (g.x+g.width)*sw, (g.y+g.height)*sh);
			addVertexToArray(vx+vi+12, cx+g.width*scale, cy,                (g.x+g.width)*sw, g.y*sh);
			cx += g.width * scale;
			vi += 16;
		}
		// Line break
		if(text[i]=='\n') {
			cx = x;
			cy += line * scale;
			//line = 0;
		}

		// End of buffer
		if(vi>=512) {
			glDrawArrays(GL_QUADS, 0, vi/4);
			vi=0;
		}
	}
	if(vi>0) glDrawArrays(GL_QUADS, 0, vi/4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	p.x=cx; p.y=cy;
	return p;
}
*/

// ================================##===============================##==================================== //


unsigned char* gui::Font::build(const char* name, int size, int width, int height) {
	m_glyphs.resize(128, Rect(0,0,0,0));
	#ifndef FREETYPE
	bool bold = false;
	#endif
	
	#ifdef LINUX // LINUX (X11)
	char str[256];
	sprintf(str, "-*-%s-%s-r-*-*-*-%i-75-75-*-*-*-*", name, bold?"bold":"medium", size*10);
	Display* display = XOpenDisplay(0);
	XFontStruct* font = XLoadQueryFont(display, str);

	// Error
	if(!font) {
		::printf("Failed to create font %s\n", name);
		font = XLoadQueryFont(display, "fixed");
		m_size = font->max_bounds.ascent; // fallback font may have a fifferent size
	}
	else ::printf("Created font %s\n", name);

	// Need to use the game window to draw to. Would be nicer to use a emporary drawable
	//Display* drawable = base::Game::window()->getXDisplay();
	Window drawable = XDefaultRootWindow(display);

	// Generate font bitmap
	XGCValues values;
	Pixmap pix = XCreatePixmap(display, drawable, width, height, 32);
	GC gc = XCreateGC(display, pix, 0, &values);
	XSetFont(display, gc, font->fid);
	XSetForeground(display, gc, BlackPixel(display, 0));
	XFillRectangle(display, pix, gc, 0, 0, width, height);
	XSetForeground(display, gc, WhitePixel(display, 0));

	// Render font
	int x = 0;
	int y = font->max_bounds.ascent;
	int h = font->max_bounds.ascent + font->max_bounds.descent;
	m_glyphHeight = h;
	for(int i=32; i<128; ++i) {
		char c = i;
		int w = XTextWidth(font, &c, 1);
		if(x + w > width) x=0, y+=h;

		m_glyphs[i].x = x;
		m_glyphs[i].y = y - font->max_bounds.ascent;
		m_glyphs[i].width = w;
		m_glyphs[i].height = h;
		XDrawString(display, pix, gc, x, y, &c, 1);
		x += w;
	}

	// Extract single channel
	XImage* img = XGetImage(display, pix, 0, 0, width, height, AllPlanes, ZPixmap);
	unsigned char* data = new unsigned char[width * height * 4];
	for(int i=0; i<width*height; ++i) {
		data[i*4] = data[i*4+1] = data[i*4+2] = 0xff;
		data[i*4+3] = img->data[i*4];
	}

	// Cleanup
	XFreePixmap(display, pix);
	XFreeGC(display, gc);
	XFreeFont(display, font);
	XCloseDisplay(display);
	return data;
	#endif
	

	// ===================================================================================================== //
	// ===================================================================================================== //


	#ifdef WIN32

	// Handle unicode
	#ifdef UNICODE
	typedef wchar_t char_t;
	wchar_t fontName[64];
	mbstowcs(fontName, name, strlen(name));
	#else
	typedef char char_t;
	const char* fontName = name;
	#endif

	// Create windows font object
	HFONT font = CreateFont( -size, 0,0,0, bold?FW_BOLD:FW_REGULAR, FALSE,FALSE,FALSE,
							ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
							FF_DONTCARE | DEFAULT_PITCH, fontName);
	
	// Need window handle to create drawable. It is possible I dont need this.
	HDC windowHDC = GetDC(GetDesktopWindow()); //base::Game::window()->getHDC();

	// Create system data
	HDC memDC = CreateCompatibleDC(windowHDC);
	HFONT oldFont = (HFONT)SelectObject(memDC, font);
	HBITMAP bmp = CreateCompatibleBitmap(windowHDC, width, height);
	HBITMAP oldBmp = (HBITMAP) SelectObject(memDC, bmp);
	TEXTMETRIC metrics;
	GetTextMetrics(memDC, &metrics);
	SetBkColor(memDC, 0x00000000);
	SetTextColor(memDC, 0x00ffffff);
	
	// Render font
	int x=0, y=0, w=0, h=metrics.tmHeight;
	m_glyphHeight = h;
	for(int i=32; i<128; ++i) {
		char_t c = i;
		GetCharWidth(memDC, c, c, &w);
		if(x+w>width) x=0, y+=h;
		TextOut(memDC, x, y, &c, 1);

		m_glyphs[i].x = x;
		m_glyphs[i].y = y;
		m_glyphs[i].width = w;
		m_glyphs[i].height = h;
		x += w;
	}

	// Get data
	BITMAPINFO info = {0};
	info.bmiHeader.biSize = sizeof(info.bmiHeader);
	int r = GetDIBits(memDC, bmp, 0, 0, 0, &info, DIB_RGB_COLORS);
	if(r==0) ::printf("Font fail\n");
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biHeight = -info.bmiHeader.biHeight;	// Flip image
	unsigned char* imgData = new unsigned char[width*height*4];
	r = GetDIBits(memDC, bmp, 0, height, imgData, &info, DIB_RGB_COLORS);
	if(r==0) ::printf("Font fail\n");
	
	// Extract single channel
	unsigned char* data = new unsigned char[width * height * 4];
	for(int i=0; i<width*height; ++i) {
		data[i*4] = data[i*4+1] = data[i*4+2] = 0xff;
		data[i*4+3] = imgData[i*4];
	}
	
	// Cleanup
	delete [] imgData;
	SelectObject(memDC, oldFont);
	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteObject(font);
	DeleteDC(memDC);
	return data;
	#endif

	// ===================================================================================================== //
	// ===================================================================================================== //
	

	#ifdef FREETYPE
	FT_Library  library;
	FT_Face     face;
	int error = 0;
	unsigned char* data = 0;

	error = FT_Init_FreeType( &library );
	if(error) { printf("Error initilising freetype\n"); return 0; }

	error = FT_New_Face(library, name, 0, &face); // Note: 0 is the face index, a file can contain multiple faces
	if(error) printf("Error: Failed to load font %s\n", name);
	else {
		// Set font size
		if(face->face_flags & FT_FACE_FLAG_SCALABLE) {
			FT_F26Dot6 ftSize = (FT_F26Dot6)(size * (1<<6));
			FT_Set_Char_Size(face, ftSize, 0, 100, 100);
		}
		else printf("Error: Font %s not scalable\n", name);

		int ascent = face->size->metrics.ascender >> 6;
		int descent = face->size->metrics.descender >> 6;
		if(TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(face, ft_sfnt_os2)) {
			auto max = [](int& v, int n) { if(n>v) v=n; };
			max(ascent,  os2->usWinAscent * face->size->metrics.y_ppem / face->units_per_EM);
			max(descent, os2->usWinDescent * face->size->metrics.y_ppem / face->units_per_EM);
			max(ascent,  os2->sTypoAscender * face->size->metrics.y_ppem / face->units_per_EM);
			max(descent, -os2->sTypoDescender * face->size->metrics.y_ppem / face->units_per_EM);
		}
		m_glyphHeight = ascent + descent;
		
		Point pos;
		data = new unsigned char[width * height * 4];
		unsigned char* end = data + width * height * 4;
		for(unsigned char* p = data; p<end; p+=4) p[0]=p[1]=p[2]=0xff, p[3]=0;
		for(int i=32; i<127; ++i) {
			FT_UInt index = FT_Get_Char_Index(face, i);
			if(FT_Load_Glyph(face, index, FT_LOAD_DEFAULT | FT_LOAD_RENDER)==0) {
				if(face->glyph->bitmap.buffer) {
					//float bearingX = face->glyph->metrics.horiBearingX / 64.f;

					// Copy out glyph pixels
					int w = face->glyph->bitmap.width;
					int h = face->glyph->bitmap.rows;
					int pitch = face->glyph->bitmap.pitch;
					if(pos.x+w > width) pos.set(0, pos.y + m_glyphHeight);


					int left = 0; //face->glyph->bitmap_left;
					int top = ascent - face->glyph->bitmap_top;

					//printf("%c : %f [%d,%d %dx%d %d]\n", (char)i, bearingX, left, top, w, h, pitch);

					unsigned char* o = data + (pos.x + left + (pos.y+top)*width) * 4 + 3;
					for(int y=0; y<h; ++y, o+=width*4) for(int x=0; x<w; ++x) {
						o[x*4] = face->glyph->bitmap.buffer[x + y*pitch];
					}
					
					m_glyphs[i].x = pos.x;
					m_glyphs[i].y = pos.y;
					m_glyphs[i].width = w;
					m_glyphs[i].height = m_glyphHeight;
					pos.x += w;
				}
				else if(face->glyph->metrics.horiAdvance > 0) {
					// Space has no bitmap data
					int w = face->glyph->metrics.horiAdvance >> 6;
					m_glyphs[i].x = pos.x;
					m_glyphs[i].y = pos.y;
					m_glyphs[i].width = w;
					m_glyphs[i].height = m_glyphHeight;
					pos.x += w;
				}
			}
			else printf("No glyph for '%c'\n", (char)i);
		}
	}

	FT_Done_FreeType(library);
	return data;
	#endif

	return 0;
}


// ================================##===============================##==================================== //


int gui::Font::findGlyphs(char* data, int w, int h, int bpp, const char* chars) {
	m_glyphs.resize(128, Rect(0,0,0,0));
	m_size = 0;
	m_w = w; m_h = h;		// Set size
	size_t stride = bpp/8;	// Pixel stride
	unsigned guide, clear=0;	// Colours
	memcpy(&guide, data, stride); // First pixel is the guide colour

	// Loop through image
	int count=0; // Number of glyphs read
	int index=0; // index of character being read
	int open=0;  // index of last open glyph
	int pixel=0, last=0;
	Rect* lastGlyph = 0; // Last glyph started
	size_t row = w*stride;
	for(int y=0; y<h; ++y) {
		last = 0;
		for(int x=0; x<w; ++x, last=pixel) {
			size_t k = x*stride + y*w*stride;
			pixel = memcmp(&guide, data+k, stride); // Is this pixel a guide
			if(pixel && !last) { // Start of glyph X
				if(y==0 || memcmp(&guide, data+k-row, stride)==0) {
					if(count==0) memcpy(&clear, data+k, stride); // Get clear colour
					// Which glyph is this
					if(chars && chars[index]) lastGlyph = &m_glyphs[ (int)chars[index] ];
					else if(!chars) lastGlyph = &m_glyphs[index+32];
					else lastGlyph = 0;
					// Initialise glyph
					if(lastGlyph) {
						lastGlyph->x = x;
						lastGlyph->y = y;
						++index;
					}
				}
			} else if((!pixel && last) || (pixel && x==w-1) ) { // End of glyph X
				if(lastGlyph && y==lastGlyph->y) lastGlyph->width = x - lastGlyph->x; // Set width
				if(y==h-1 || memcmp(&guide, data+k+row-stride, stride)==0) {
					// Which glyph did we close?
					for(int i=open; i<128; ++i) {
						if(x==m_glyphs[i].x+m_glyphs[i].width && !m_glyphs[i].height) {
							if(i==open) ++open;
							m_glyphs[i].height = y-m_glyphs[i].y;
							if(m_glyphs[i].height > m_size) m_size = m_glyphs[i].y; // General font size
							++count;
							break;
						}
					}
				}
			}
		}
	}
	// Remove all guides
	for(unsigned i=0; i<w*h*stride; i+=stride) {
		if(memcmp(&guide, data+i, stride)==0) memcpy(data+i, &guide, stride);
	}
	for(Rect& g: m_glyphs) if(g.height > m_glyphHeight) m_glyphHeight = g.height;
	return count;
}



