#include "image.hh"
#include <cstdlib>
#include <cmath>

Image::Image(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	pixels = (Pixel *)calloc(width * height, sizeof(Pixel));
}

Image::~Image()
{
	free(pixels);
}

void Image::FillColor(Pixel color)
{
	for (unsigned int i = 0; i < width; i++)
	{
		for (unsigned int j = 0; j < height; j++)
		{
			*(GetPixel(i, j)) = color;
		}
	}
}

Pixel Image::BlendPixels(Pixel p1, Pixel p2)
{
	Pixel newPixel;
	newPixel.r = BlendSubPixel(p1.r, p2.r, p2.a);
	newPixel.g = BlendSubPixel(p1.g, p2.g, p2.a);
	newPixel.b = BlendSubPixel(p1.b, p2.b, p2.a);
	newPixel.a = BlendAlpha(p1.a, p2.a);
	return newPixel;
}

unsigned int Image::BlendAlpha(unsigned int oldVal, unsigned int newVal)
{
	return oldVal + newVal * (255 - oldVal) / 255;
}

unsigned int Image::BlendSubPixel(unsigned int oldVal, unsigned int newVal, unsigned int newOpacity)
{
	return newVal * newOpacity + oldVal * (255 - newOpacity) / 255;
}

unsigned int Image::RenderAlphaForPixel(std::vector<Line> *lines, unsigned int x, unsigned int y)
{
	int alpha = 0;

	for (unsigned int i = 0; i < lines->size(); i++)
	{
		Line line = lines->at(i);
		int tmpAlpha = RenderAlphaForPixel(line, x, y);
		alpha = BlendAlpha(alpha, tmpAlpha);
	}

	return alpha;
}

void Image::RenderLines(Pixel color, std::vector<Line> *lines)
{
	for (unsigned int i = 0; i < lines->size(); i++)
	{
		auto line = lines->at(i);

		int tmpdx = line.end.x - line.start.x;
		int tmpdy = line.end.y - line.start.y;

		if (abs(tmpdx) < abs(tmpdy))
		{
			DrawLineXCentric(line, color);
		}
		else
		{
			DrawLineYCentric(line, color);
		}
	}
}

void Image::DrawLineXCentric(Line line, Pixel color)
{
	Point start = line.start;
	Point end = line.end;
	if (line.start.y > line.end.y)
	{
		start = line.end;
		end = line.start;
	}

	int dx = end.x - start.x;
	int dy = end.y - start.y;

	float x = (float)start.x;

	for (unsigned int y = start.y; y < end.y; y++)
	{
		DrawPixelsAt(line, color, ceil(x), y);
		DrawPixelsAt(line, color, floor(x), y);
		x = start.x + dx / (float)dy * (y - start.y);
	}
}

void Image::DrawLineYCentric(Line line, Pixel color)
{
	Point start = line.start;
	Point end = line.end;
	if (line.start.x > line.end.x)
	{
		start = line.end;
		end = line.start;
	}

	int dx = end.x - start.x;
	int dy = end.y - start.y;

	for (unsigned int x = start.x; x < end.x; x++)
	{
		float y = start.y + dy / (float)dx * (x - start.x);
		DrawPixelsAt(line, color, x, ceil(y));
		DrawPixelsAt(line, color, x, floor(y));
	}
}

void Image::DrawPixelsAt(Line line, Pixel color, unsigned int x, unsigned int y)
{
	int opacity = RenderAlphaForPixel(line, x, y);
	if (opacity > 0)
	{
		color.a = opacity;
	}
	auto originalColor = this->GetPixel(x, y);
	*originalColor = BlendPixels(*originalColor, color);
}

/// <summary>
/// Returns the opacity value to use at the given pixel
/// </summary>
/// <param name="line"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
unsigned int Image::RenderAlphaForPixel(Line line, unsigned int x, unsigned int y)
{
	int dx = line.end.x - line.start.x;
	int dy = line.end.y - line.start.y;

	int px = x - line.start.x;
	int py = y - line.start.y;

	float y1 = dy / (float)dx * (float)px;
	float dify = abs(y1 - py);
	if (dify != dify) //NaN - weird way to check
	{
		dify = 0;
	}
	if (dify >= 1 || abs(px) < 0 || abs(px) > abs(dx))
	{
		return 0;
	}
	else
	{
		return (int)round(255 * (1 - dify));
	}
}