#include "base/png.h"

#include "png/png.h"
#include <cstdlib>
#include <cstring>

#include <cstdio>

using namespace base;


struct PNGMemoryReader { const char* data; size_t size; size_t read; };
void png_read_memory(png_structp png_ptr, png_bytep data, png_size_t length) {
	PNGMemoryReader* src = (PNGMemoryReader*)png_get_io_ptr(png_ptr);
	if(!src) return;
	if(src->read + length > src->size) return;
	memcpy(data, src->data + src->read, length);
	src->read += length;
}

Image process_png_loader(png_structp png_ptr, png_infop info_ptr);

Image PNG::parse(const void* data, unsigned size) {
	if(!data || !size) return Image();
	
	// check for valid magic number
	if(!png_check_sig((const unsigned char*)data, 8)) {
		printf("Invalid PNG image\n"); 
		return Image();
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) return Image();
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return Image();
	}
	
	// initialize the setjmp for returning properly after a libpng error occured
	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return Image();
	}
	
	PNGMemoryReader source { (const char*)data, size, 8 };
	png_set_read_fn(png_ptr, &source, png_read_memory);
	return process_png_loader(png_ptr, info_ptr);
}


Image PNG::load(const char* filename) {
	FILE* fp = fopen(filename, "rb");
	if(!fp) return Image();
	
	// check for valid magic number
	png_byte magic[8];
	fread(magic, 1, 8, fp);
	if(!png_check_sig(magic, 8)) {
		printf("Invalid PNG image\n"); 
		fclose(fp);
		return Image();
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) {
		fclose(fp);
		return Image();
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return Image();
	}
	
	// initialize the setjmp for returning properly after a libpng error occured
	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return Image();
	}

	png_init_io (png_ptr, fp);

	Image image = process_png_loader(png_ptr, info_ptr);
	fclose(fp);
	return image;
}

Image process_png_loader(png_structp png_ptr, png_infop info_ptr) {
	png_set_sig_bytes(png_ptr, 8); // We have already read the signiture
	
	png_read_info(png_ptr, info_ptr);
	int mcdepth = png_get_bit_depth(png_ptr, info_ptr);
	int color_type = png_get_color_type(png_ptr, info_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	
	// convert 1-2-4 bits grayscale images to 8 bits grayscale.
	int bit_depth = 32;
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	if(bit_depth == 16) png_set_strip_16(png_ptr);
	else if(bit_depth < 8) png_set_packing(png_ptr);
	
	/* update info structure to apply transformations */
	png_read_update_info(png_ptr, info_ptr);
	
	// retrieve updated information
	unsigned int mwidth;
	unsigned int mheight;
	png_get_IHDR(png_ptr, info_ptr,
		(png_uint_32*)(&mwidth),
		(png_uint_32*)(&mheight),
		&mcdepth, &color_type,
		NULL, NULL, NULL);
	
	// get image format and components per pixel
	int iFormat=0;
	Image::Format format;
	switch (color_type) {
	case PNG_COLOR_TYPE_GRAY:
		if(bit_depth==8) {
			iFormat = 1;
			mcdepth = 8;
			format = Image::R8;
		}
		else if(bit_depth == 16) {
			format = Image::R16;
			iFormat = 2;
			mcdepth = 16;
		}
		break;
	
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		iFormat = 2;
		mcdepth = 16;
		format = Image::RG8;
		break;
	
	case PNG_COLOR_TYPE_RGB:
		iFormat = 3;
		mcdepth = 24;
		format = Image::RGB8;
		break;
	
	case PNG_COLOR_TYPE_RGB_ALPHA:
		iFormat = 4;
		mcdepth = 32;
		format = Image::RGBA8;
		break;
	
	default: break;
	}

	if(iFormat == 0) {
		printf("Invalid PNG Format\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return Image();
	}

	
	// Read the image data
	char* mdata = new char[mwidth * mheight * iFormat];
	png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * mheight);
	for(unsigned i=0; i < mheight; ++i) {
		row_pointers[i] = (png_bytep)(mdata + (i * mwidth * iFormat));
	}
	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	free(row_pointers);
	

	return Image(format, mwidth, mheight, (Image::byte*)mdata);
}

bool PNG::save(const Image& image, const char* filename) {
	if(!image) return false;
	if(image.getMode() != Image::FLAT) return false;

	FILE* fp = fopen(filename, "wb");
	unsigned iFormat = image.getBytesPerPixel();
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep *row_pointers = NULL;
	if(!fp) return false;

	/** Initialise write struct */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) {
		fclose(fp);
		return false;
	}

	/** Initialise info struct */
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return false;
	}
	
	/** Set up error handling */
	if(setjmp( png_jmpbuf(png_ptr) )) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return false;
	}

	int format = 0;
	switch(image.getFormat()) {
	case Image::R8:    format = PNG_COLOR_TYPE_GRAY; break;
	case Image::RG8:   format = PNG_COLOR_TYPE_GRAY_ALPHA; break;
	case Image::RGB8:  format = PNG_COLOR_TYPE_RGB; break;
	case Image::RGBA8: format = PNG_COLOR_TYPE_RGB_ALPHA; break;
	default:
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}

	/** Set image attributes */
	int width = image.getWidth();
	int height = image.getHeight();
	png_set_IHDR( png_ptr, info_ptr, width, height, 8, format,
	              PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	/** Initialise rows */
	row_pointers = (png_bytep*) malloc (sizeof (png_bytep) * height);
	for(int i=0; i<height; ++i) {
		row_pointers[i] = (png_bytep)(image.getData() + (i * width * iFormat));
	}

	/** Write image data */
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	/** cleanup */
	free(row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	return true;
}

