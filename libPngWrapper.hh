#pragma once

#include "image.hh"
#include <png.h>


class LibPngWrapper
{
public:
	const char* filename;

	LibPngWrapper(const char* filename);
	~LibPngWrapper();

	bool WriteImage(Image* image);

private:
	int code = 0;
	FILE *fp = nullptr;
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
};

