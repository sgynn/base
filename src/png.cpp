#include "base/png.h"

#include "png/png.h"
#include <cstdlib>
#include <cstring>

#include <cstdio>

using namespace base;

PNG::PNG(const PNG& p) : data(0), width(p.width), height(p.height), bpp(p.bpp) {
	if(p.data) {
		size_t size = sizeof(unsigned char) * width * height * bpp/8;
		data = (char*)malloc( size );
		memcpy(data, p.data, size);
	}
}
const PNG& PNG::operator=(const PNG& p) {
	width=p.width; height=p.height; bpp=p.bpp;
	if(p.data) {
		size_t size = sizeof(unsigned char) * width * height * bpp/8;
		data = (char*)malloc( size );
		memcpy(data, p.data, size);
	}
	else data = 0;
	return *this;
}

PNG::PNG(PNG&& p) : data(p.data), width(p.width), height(p.height), bpp(p.bpp) { p.data = 0; }
const PNG& PNG::operator=(PNG&& p) {
	data = p.data;
	width = p.width;
	height = p.height;
	bpp = p.bpp;
	p.data = 0;
	return *this;
}

/*
//Custom read handler for PNG library so that it uses our File object.
void PNGAPI png_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	png_size_t check;
	if(png_ptr == NULL) return;
	FILE *fp = (FILE*)png_ptr->io_ptr;
	check = (png_size_t) fread((char*)data, 1, length, fp); 
	if(check != length) png_error(png_ptr, "Read Error");
}
*/

PNG PNG::load(const char* filename) {
	png_byte magic[8];
	png_structp png_ptr;
	png_infop info_ptr;
	int bit_depth, color_type;
	png_bytep *row_pointers = NULL;
	unsigned int i;

	char *mdata;
	unsigned int mwidth;
	unsigned int mheight;
	int mcdepth;
	
	FILE* fp = fopen(filename, "rb");
	if(!fp) return PNG();
	
	/* read magic number */
	size_t r = fread (magic, 1, sizeof(magic), fp);

	/* check for valid magic number */
	if (r!=sizeof(magic) || !png_check_sig(magic, sizeof(magic))) {
		printf("Invalid PNG image\n"); 
		fclose(fp);
		return PNG();
	}
	
	/* create a png read struct */
	png_ptr = png_create_read_struct
	(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		return PNG();
	}
	
	/* create a png info struct */
	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		fclose(fp);
		return PNG();
	}
	
	/* create our OpenGL texture object */
	//texinfo = (gl_texture_t *)malloc (sizeof (gl_texture_t));
	
	/* initialize the setjmp for returning properly after a libpng
	error occured */
	if (setjmp (png_jmpbuf (png_ptr))) {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		if (row_pointers) free (row_pointers);
		fclose(fp);
		return PNG();
	}
	
	/* setup libpng for using standard C fread() function with our FILE pointer */
	//THIS LINE IS BROKEN!!!!!!
	//png_set_read_fn(png_ptr, 0, png_read_data);
	png_init_io (png_ptr, fp);
	
	/* tell libpng that we have already read the magic number */
	png_set_sig_bytes (png_ptr, sizeof (magic));
	
	/* read png info */
	png_read_info (png_ptr, info_ptr);
	
	/* get some usefull information from header */
	mcdepth = png_get_bit_depth (png_ptr, info_ptr);
	color_type = png_get_color_type (png_ptr, info_ptr);
	
	/* convert index color images to RGB images */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_palette_to_rgb (png_ptr);
	
	/* convert 1-2-4 bits grayscale images to 8 bits grayscale. */
	bit_depth = 32;
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8 (png_ptr);
	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha (png_ptr);
	if (bit_depth == 16) png_set_strip_16 (png_ptr);
	else if (bit_depth < 8) png_set_packing (png_ptr);
	
	/* update info structure to apply transformations */
	png_read_update_info (png_ptr, info_ptr);
	
	/* retrieve updated information */
	png_get_IHDR (png_ptr, info_ptr,
		(png_uint_32*)(&mwidth),
		(png_uint_32*)(&mheight),
		&mcdepth, &color_type,
		NULL, NULL, NULL);
	
	/* get image format and components per pixel */
	int iFormat=0;
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GRAY:
	iFormat = 1;
	mcdepth = 8;
	break;
	
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	iFormat = 2;
	mcdepth = 16;
	break;
	
	case PNG_COLOR_TYPE_RGB:
	iFormat = 3;
	mcdepth = 24;
	break;
	
	case PNG_COLOR_TYPE_RGB_ALPHA:
	iFormat = 4;
	mcdepth = 32;
	break;
	
	default:
	/* Badness */
	return PNG();
	break;
	}
	
	/* we can now allocate memory for storing pixel data */
	mdata = (char*)malloc (sizeof(unsigned char) * mwidth * mheight * iFormat);
	
	/* setup a pointer array.  Each one points at the begening of a row. */
	row_pointers = (png_bytep *)malloc (sizeof (png_bytep) * mheight);
	
	for (i = 0; i < mheight; ++i) {
		//row_pointers[i] = (png_bytep)(mdata + ((mheight - (i + 1)) * mwidth * iFormat));
		row_pointers[i] = (png_bytep)(mdata + (i * mwidth * iFormat));
	}
	
	/* read pixel data using row pointers */
	png_read_image (png_ptr, row_pointers);
	
	/* finish decompression and release memory */
	png_read_end (png_ptr, info_ptr);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	
	/* we don't need row pointers anymore */
	free (row_pointers);
	
	fclose(fp);

	PNG png;
	png.data = mdata;
	png.width = mwidth;
	png.height = mheight;
	png.bpp = mcdepth;

	return png;
}


int PNG::save(const char* filename) {
	if(!width || !height || !bpp || !data) return -2; // Invalid image data
	FILE* fp = fopen(filename, "wb");
	unsigned iFormat = bpp/8;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers = NULL;
	if(!fp) return -1;

	/** Initialise write struct */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) {
		fclose(fp);
		return -1;
	}

	/** Initialise info struct */
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return -1;
	}
	
	/** Set up error handling */
	if(setjmp( png_jmpbuf(png_ptr) )) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return -1;
	}

	int format = 0;
	switch(iFormat) {
	case 1: format = PNG_COLOR_TYPE_GRAY; break;
	case 2: format = PNG_COLOR_TYPE_GRAY_ALPHA; break;
	case 3: format = PNG_COLOR_TYPE_RGB; break;
	case 4: format = PNG_COLOR_TYPE_RGB_ALPHA; break;
	default:
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return -1;
	}

	/** Set image attributes */
	png_set_IHDR( png_ptr, info_ptr, width, height, 8, format,
	              PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	/** Initialise rows */
	row_pointers = (png_bytep*) malloc (sizeof (png_bytep) * height);
	for(int i=0; i<height; ++i) {
		row_pointers[i] = (png_bytep)(data + (i * width * iFormat));
	}

	/** Write image data */
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/** cleanup */
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return 0;
}

PNG PNG::create(int width, int height, int depth, const char* data) {
	PNG png;
	png.width = width;
	png.height = height;
	png.bpp = depth * 8;
	size_t size = sizeof(char) * width * height * depth;
	png.data = (char*) malloc( size );
	if(data) memcpy(png.data, data, size);
	else memset(png.data, 0xff, size);
	return png;
}

void PNG::clear() {
	if(data) free(data);
	width=height=bpp=0;
	data = 0;
}

