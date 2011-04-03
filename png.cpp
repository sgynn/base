#include "png.h"

#include "png/png.h"
#include <cstdlib>
#include <cstring>

#include <cstdio>

using namespace base;

//Custom read handler for PNG library so that it uses our File object.
void PNGAPI png_read_data(png_structp png_ptr, png_bytep data, png_size_t length) {
	png_size_t check;
	if(png_ptr == NULL) return;
	FILE *fp = (FILE*)png_ptr->io_ptr;
	check = (png_size_t) fread((char*)data, 1, length, fp); 
	if(check != length) png_error(png_ptr, "Read Error");
}

bool PNG::load(const char* filename) {
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
	if(!fp) return false;
	
	/* read magic number */
	fread (magic, 1, sizeof(magic), fp);

	/* check for valid magic number */
	if (!png_check_sig (magic, sizeof (magic))) {
		printf("Invalid PNG image\n"); 
		fclose(fp);
		return false;
	}
	
	/* create a png read struct */
	png_ptr = png_create_read_struct
	(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		return false;
	}
	
	/* create a png info struct */
	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, NULL, NULL);
		fclose(fp);
		return false;
	}
	
	/* create our OpenGL texture object */
	//texinfo = (gl_texture_t *)malloc (sizeof (gl_texture_t));
	
	/* initialize the setjmp for returning properly after a libpng
	error occured */
	if (setjmp (png_jmpbuf (png_ptr))) {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		if (row_pointers) free (row_pointers);
		fclose(fp);
		return false;
	}
	
	/* setup libpng for using standard C fread() function with our FILE pointer */
	//THIS LINE IS BROKEN!!!!!!
	png_set_read_fn(png_ptr, 0, png_read_data);
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
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8 (png_ptr);
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
	iFormat=0;
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GRAY:
	iFormat = 1;
	mcdepth = 8;
	break;
	
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	iFormat = 2;
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
	break;
	}
	
	/* we can now allocate memory for storing pixel data */
	mdata = (char*)malloc (sizeof(unsigned char) * mwidth * mheight * iFormat);
	
	/* setup a pointer array.  Each one points at the begening of a row. */
	row_pointers = (png_bytep *)malloc (sizeof (png_bytep) * mheight);
	
	for (i = 0; i < mheight; ++i) {
		row_pointers[i] = (png_bytep)(mdata + ((mheight - (i + 1)) * mwidth * iFormat));
	}
	
	/* read pixel data using row pointers */
	png_read_image (png_ptr, row_pointers);
	
	/* finish decompression and release memory */
	png_read_end (png_ptr, info_ptr);
	png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
	
	/* we don't need row pointers anymore */
	free (row_pointers);
	
	fclose(fp);

	data = mdata;
	width = mwidth;
	height = mheight;
	bpp = mcdepth;

	flip(); //Flip pixels in Y axis

	return true;
}

void PNG::flip() {
	char* mdata = data;
	data = (char*)malloc (sizeof(unsigned char) * width * height * iFormat);
	for(int i=0; i<height; i++) {
		int a = (height-i-1) * width * iFormat;
		memcpy(data+(i*width*iFormat), mdata+a, width*iFormat);
	}
	free(mdata);
}

void PNG::clear() {
	if(data) delete [] data;
	data = 0;
	width=height=0;
}

