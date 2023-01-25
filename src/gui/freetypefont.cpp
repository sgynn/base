#include <base/gui/font.h>
#include <cstring>
#include <cstdio>


gui::FreeTypeFont::FreeTypeFont(const char* file) {
	strncpy(m_file, file, sizeof(m_file));
}


#ifdef FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

bool gui::FreeTypeFont::build(int size) {
	if(m_glyphs.empty()) return false;

	Font* font = 0;
	FT_Library  library;
	FT_Face     face;
	int error = 0;

	error = FT_Init_FreeType( &library );
	if(error) { printf("Error initilising freetype\n"); return false; }

	error = FT_New_Face(library, file, 0, &face); // Note: 0 is the face index, a file can contain multiple faces
	if(error) printf("Error: Failed to load font %s\n", file);
	else {
		// Set font size
		if(face->face_flags & FT_FACE_FLAG_SCALABLE) {
			FT_F26Dot6 ftSize = (FT_F26Dot6)(size*0.75) << 6;
			FT_Set_Char_Size(face, ftSize, 0, 100, 100);
		}
		else printf("Error: Font %s not scalable\n", file);

		int ascent = face->size->metrics.ascender >> 6;
		int descent = face->size->metrics.descender >> 6;
		if(TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(face, ft_sfnt_os2)) {
			auto max = [](int& v, int n) { if(n>v) v=n; };
			max(ascent,  os2->usWinAscent * face->size->metrics.y_ppem / face->units_per_EM);
			max(descent, os2->usWinDescent * face->size->metrics.y_ppem / face->units_per_EM);
			max(ascent,  os2->sTypoAscender * face->size->metrics.y_ppem / face->units_per_EM);
			max(descent, -os2->sTypoDescender * face->size->metrics.y_ppem / face->units_per_EM);
		}

		// Guess a reasonable texture size
		Point imageSize = selectImageSize(size, countGlyphs());
		int width = imageSize.x;
		int height = imageSize.y;
		count = 0;

		createFace(size, ascent+descent);
		allocateGlyphs();

		printf("Font metrics: %d : %dx%d %d %d\n", size, width, height, ascent, descent);
		Rect rect(0,0,0,ascent+descent);
		unsigned char* data = new unsigned char[width * height * 4];
		unsigned char* end = data + width * height * 4;
		for(unsigned char* p = data; p<end; p+=4) p[0]=p[1]=p[2]=0xff, p[3]=0;

		for(const Range& range : m_glyphs) {
			for(int glyph=range.start; glyph<=range.end; ++glyph) {
				FT_UInt index = FT_Get_Char_Index(face, glyph);
				if(FT_Load_Glyph(face, index, FT_LOAD_DEFAULT | FT_LOAD_RENDER)==0) {
					if(face->glyph->bitmap.buffer) {
						//float bearingX = face->glyph->metrics.horiBearingX / 64.f;

						// Copy out glyph pixels
						rect.width = face->glyph->bitmap.width;
						int h = face->glyph->bitmap.rows;
						int pitch = face->glyph->bitmap.pitch;
						if(rect.right() > width) { rect.x = 0; rect.y += rect.height; }

						int left = 0; //face->glyph->bitmap_left;
						int top = ascent - face->glyph->bitmap_top;

						//printf("%c : %f [%d,%d %dx%d %d]\n", (char)i, bearingX, left, top, w, h, pitch);

						unsigned char* o = data + (rect.x + left + (rect.y+top)*width) * 4 + 3;
						for(int y=0; y<h; ++y, o+=width*4) for(int x=0; x<rect.width; ++x) {
							o[x*4] = face->glyph->bitmap.buffer[x + y*pitch];
						}
						++count;

					}
					else if(face->glyph->metrics.horiAdvance > 0) {
						// Space has no bitmap data
						rect.width = face->glyph->metrics.horiAdvance >> 6;
					}
					setGlyph(font, glyph, rect);
					rect.x += rect.width;
				}
				else printf("No glyph for '%c'\n", (char)glyph);
			}
		}
		addImage(width, height, data);
		printf("Loaded font %s %dx%d %d glyphs\n", file, width, height, count);
		delete [] data;
	}
	FT_Done_FreeType(library);
	return error == 0;
}

#else
bool gui::FreeTypeFont::build(int size) {
	printf("Error: Baselib not compiled with freetype\n");
	return false;
}
#endif

