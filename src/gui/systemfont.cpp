#include <base/gui/font.h>
#include <cstdio>

#ifdef LINUX
#include <X11/Xlib.h>
#endif

#ifdef WIN32
#include <base/game.h>
#include <base/window.h>
#endif

#ifdef LINUX	// Linux X11
gui::Font* gui::SystemFont::load(const char* name, int size, const gui::GlyphRangeVector& glyphs) {
	if(glyphs.empty()) return 0;
	Font* result = 0;
	bool bold = false;
	
	char str[256];
	sprintf(str, "-*-%s-%s-r-*-*-*-%i-75-75-*-*-*-*", name, bold?"bold":"medium", size*10);
	Display* display = XOpenDisplay(0);
	XFontStruct* font = XLoadQueryFont(display, str);

	// Error
	if(!font) {
		printf("Failed to create font %s - Using fallback\n", name);
		font = XLoadQueryFont(display, "fixed");
		size = font->max_bounds.ascent; // fallback font may have a different size
	}
	else printf("Created font %s\n", name);

	// Guess a reasonable texture size
	int count = 0;
	for(const GlyphRange& range : glyphs) count += range.max - range.min + 1;
	Point imageSize = selectImageSize(size, count);
	int width = imageSize.x;
	int height = imageSize.y;


	// Generate font bitmap
	Window drawable = XDefaultRootWindow(display);
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
	result = createFontObject(size, h);
	Rect rect;
	for(const GlyphRange& range : glyphs) {
		for(unsigned i=range.min; i<=range.max; ++i) {
			char c = i;
			int w = XTextWidth(font, &c, 1);
			if(x + w > width) x=0, y+=h;
			rect.x = x;
			rect.y = y - font->max_bounds.ascent;
			rect.width = w;
			rect.height = h;
			setGlyph(result, i, rect);
			XDrawString(display, pix, gc, x, y, &c, 1);
			x += w;
		}
	}

	// Extract single channel
	XImage* img = XGetImage(display, pix, 0, 0, width, height, AllPlanes, ZPixmap);
	unsigned char* data = new unsigned char[width * height * 4];
	for(int i=0; i<width*height; ++i) {
		data[i*4] = data[i*4+1] = data[i*4+2] = 0xff;
		data[i*4+3] = img->data[i*4];
	}
	setFontImage(result, width, height, data);

	// Cleanup
	XFreePixmap(display, pix);
	XFreeGC(display, gc);
	XFreeFont(display, font);
	XCloseDisplay(display);
	delete [] data;
	return result;
}
#endif

// ===================================================================================================== //
// ===================================================================================================== //


#ifdef WIN32
gui::Font* gui::SystemFont::load(const char* name, int size, const gui::GlyphRangeVector& glyphs) {
	if(glyphs.empty()) return 0;

	// Handle unicode
	#ifdef UNICODE
	typedef wchar_t char_t;
	wchar_t fontName[64];
	mbstowcs(fontName, name, strlen(name));
	#else
	typedef char char_t;
	const char* fontName = name;
	#endif

	// Guess a reasonable texture size
	int count = 0;
	for(const GlyphRange& range : glyphs) count += range.max - range.min + 1;
	Point imageSize = selectImageSize(size, count);
	int width = imageSize.x;
	int height = imageSize.y;

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
	Rect rect(0,0,0,metrics.tmHeight);
	Font* result = createFontObject(size, rect.height);
	for(const GlyphRange& range : glyphs) {
		for(unsigned i=range.min; i<=range.max; ++i) {
			char_t c = i;
			GetCharWidth(memDC, c, c, &rect.width);
			if(rect.right() > width) rect.x=0, rect.y += rect.height;
			TextOut(memDC, rect.x, rect.y, &c, 1);
			rect.x += rect.width;
			setGlyph(result, i, rect);
		}
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
	if(r) {
		// Extract single channel
		unsigned char* data = new unsigned char[width * height * 4];
		for(int i=0; i<width*height; ++i) {
			data[i*4] = data[i*4+1] = data[i*4+2] = 0xff;
			data[i*4+3] = imgData[i*4];
		}
		setFontImage(result, width, height, data);
		delete [] data;
	}
	else {
		printf("Font fail\n");
		delete result;
		result = 0;
	}
	
	// Cleanup
	delete [] imgData;
	SelectObject(memDC, oldFont);
	SelectObject(memDC, oldBmp);
	DeleteObject(bmp);
	DeleteObject(font);
	DeleteDC(memDC);
	return result;
}
#endif

#if !defined(LINUX) && !defined(WIN32)
gui::Font* gui::SystemFont::load(const char* file, int size, const gui::GlyphRangeVector& glyphs) {
	printf("Error: No system font availiable\n");
	return 0;
}
#endif
