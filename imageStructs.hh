#pragma once

struct Pixel
{
	Pixel(char r, char g, char b, char a = 255)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	Pixel(){};

	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 255;
	unsigned char a = 255;
};

struct Point
{
	unsigned int x;
	unsigned int y;
};

struct Line
{
	Point start;
	Point end;
};
