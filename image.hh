#pragma once
#include <vector>
#include "imageStructs.hh"

class Image
{
  public:
	unsigned int width;
	unsigned int height;
	Pixel *pixels;

	inline Pixel *GetPixel(unsigned int x, unsigned int y)
	{
		if (x > width)
			throw "x is outside of image!";

		if (y > height)
			throw "y is outside of image!";

		return &pixels[y * width + x];
	}

	Image(unsigned int width, unsigned int height);
	~Image();

	void FillColor(Pixel color);

	/// <summary>
	/// Returns the opacity value to use at the given pixel
	/// </summary>
	/// <param name="line"></param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <returns></returns>

	void RenderLines(Pixel color, std::vector<Line> *lines);
	void DrawLineXCentric(Line line, Pixel color);
	void DrawLineYCentric(Line line, Pixel color);
	void DrawPixelsAt(Line line, Pixel color, unsigned int x, unsigned int y);

	static unsigned int RenderAlphaForPixel(Line line, unsigned int x, unsigned int y);
	static unsigned int RenderAlphaForPixel(std::vector<Line> *lines, unsigned int x, unsigned int y);
	static Pixel BlendPixels(Pixel p1, Pixel p2);
	static unsigned int BlendSubPixel(unsigned int oldVal, unsigned int newVal, unsigned int newAlpha = 255);
	static unsigned int BlendAlpha(unsigned int oldVal, unsigned int newVal);
};
