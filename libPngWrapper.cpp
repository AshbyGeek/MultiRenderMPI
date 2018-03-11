#include "libPngWrapper.hh"

#include <iostream>


LibPngWrapper::LibPngWrapper(const char* filename)
{
	this->filename = filename;

	this->fp = fopen(filename, "wb");
	if (this->fp == nullptr)
	{
		std::cerr << "Could not open file " << filename << " for writing\n";
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == nullptr)
	{
		std::cerr << "Could not allocate png write struct\n";
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == nullptr)
	{
		std::cerr << "Could not allocate info struct\n";
	}
}

bool LibPngWrapper::WriteImage(Image* image)
{
	png_image pngImage;
	pngImage.width = image->width;
	pngImage.height = image->height;

	if (fp == nullptr || png_ptr == nullptr || info_ptr == nullptr || setjmp(png_jmpbuf(png_ptr)))
	{
		std::cerr << "Error while writing file contents";
		return false;
	}

	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, 
				 info_ptr, 
				 image->width, image->height, 
				 8,
				 PNG_COLOR_TYPE_RGBA, 
				 PNG_INTERLACE_NONE, 
				 PNG_COMPRESSION_TYPE_DEFAULT, 
				 PNG_FILTER_TYPE_DEFAULT);

	png_text title_text;
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = (png_charp)"Title";
	title_text.text = (png_charp)filename;
	png_set_text(png_ptr, info_ptr, &title_text, 1);

	png_write_info(png_ptr, info_ptr);
	for (unsigned int y = 0; y < image->height; y++)
	{
		png_write_row(png_ptr, (png_bytep)image->GetPixel(0, y));
	}

	png_write_end(png_ptr, NULL);
	return true;
}

LibPngWrapper::~LibPngWrapper()
{
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
}
