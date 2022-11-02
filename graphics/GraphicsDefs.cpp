/*
 * GraphicsDefs.cpp
 *
 *  Created on: 03/apr/2015
 *      Author: stefano
 */

#include "GraphicsDefs.h"

#include "Bitmap.h"

#include <iostream>
#include <SDL.h>

namespace GFX {

	// I put these here but don't belong
GFX::Palette* kPaletteRed;
GFX::Palette* kPaletteBlue;
GFX::Palette* kPaletteYellow;
GFX::Palette* kPaletteBlack;

const GFX::point kOrigin = { 0, 0 };


// GFX::point
point::point()
	:
	x(0),
	y(0)
{
}


point::point(int16 _x, int16 _y)
	:
	x(_x),
	y(_y)
{
}


void
point::Print() const
{
	std::cout << "x: " << x << ", y: " << y << std::endl;
}


point
operator+(const point& pointA, const point& pointB)
{
	return point(pointA.x + pointB.x, pointA.y + pointB.y);
}


point
operator-(const point& pointA, const point& pointB)
{
	return point(pointA.x - pointB.x, pointA.y - pointB.y);
}


// GFX::rect
rect::rect()
{
	x = y = w = h = 0;
}


rect::rect(sint16 _x, sint16 _y, uint16 width, uint16 height)
	:
	x(_x),
	y(_y),
	w(width),
	h(height)
{
}



GFX::point
rect::LeftTop() const
{
	GFX::point point = {x, y};
	return point;
}


GFX::point
rect::Center() const
{
	return GFX::point(x + w / 2, y + h / 2);
}


GFX::point
rect::RightBottom() const
{
	return GFX::point(x + w, y + h);
}


void
rect::Print() const
{
	std::cout << std::dec;
	std::cout << "Rect: x: " << x << ", y: " << y;
	std::cout << ", w: " << w << ", h: " << h << std::endl;
}


void
GFXRectToSDLRect(const rect* source, SDL_Rect* dest)
{
	dest->x = (int)source->x;
	dest->y = (int)source->y;
	dest->w = (int)source->w;
	dest->h = (int)source->h;
}


void
SDLRectToGFXRect(const SDL_Rect* source, rect* dest)
{
	dest->x = (sint16)source->x;
	dest->y = (sint16)source->y;
	dest->w = (uint16)source->w;
	dest->h = (uint16)source->h;
}


Palette::Palette()
{
}


Palette::Palette(const GFX::Palette& palette)
{
	*this = palette;
}


Palette::Palette(const GFX::Color& start, const GFX::Color& end)
{
	// TODO: this is used only by text rendering, at the moment.
	// in theory should be a gradient, but for text it doesn't work
	colors[0].r = 0;
	colors[0].g = 255;
	colors[0].b = 0;
	colors[0].a = 0;
	for (int c = 1; c < 256; c++) {
		colors[c].r = end.r + (uint8)(((start.r - end.r) * c ) / 255);
		colors[c].g = end.g + (uint8)(((start.g - end.g) * c ) / 255);
		colors[c].b = end.b + (uint8)(((start.b - end.b) * c ) / 255);
		colors[c].a = end.a + (uint8)(((start.a - end.a) * c ) / 255);
	}
}


Palette&
Palette::operator=(const Palette& palette)
{
	for (int c = 0; c < 256; c++) {
		colors[c] = palette.colors[c];
	}

	return *this;
}


void
Palette::ModColor(uint8 index, uint8 mod)
{
	colors[index].r = colors[mod].r;
	colors[index].g = colors[mod].g;
	colors[index].b = colors[mod].b;
	//colors[index].a = colors[index].a + mod;
}


void
Palette::Dump() const
{
	std::cout << "Palette:" << std::endl;
	for (int c = 0; c < 256; c++) {
		std::cout << std::dec;
		std::cout << "color " << c << ":" << std::endl;
		std::cout << (int)colors[c].r << ", " << (int)colors[c].g << ", ";
		std::cout << (int)colors[c].b << ", " << (int)colors[c].a << std::endl;
	}
}


static void
FillBlock(Bitmap* bitmap, uint16 x, uint16 y, uint32 color)
{
	GFX::rect rect(x, y, 8, 8);
	bitmap->FillRect(rect, color);
}


void
Palette::Dump(const char* fileName) const
{
	int blockSize = 8;

	Bitmap* bitmap = new Bitmap(16 * blockSize, 16 * blockSize, 8);
	bitmap->SetPalette(*this);
	int c = 0;
	for (int y = 0; y < bitmap->Height(); y += blockSize) {
		for (int x = 0; x < bitmap->Width(); x += blockSize) {
			uint32 color = bitmap->MapColor(colors[c].r, colors[c].g, colors[c].b);
			FillBlock(bitmap, x, y, color);
			c++;
		}
	}
	bitmap->Save(fileName);
	bitmap->Release();
}


bool
InitializeGlobalPalettes()
{
	const GFX::Color kTransparentColor = { 0, 255, 0, 0 };
	try {
		kPaletteRed = new GFX::Palette();
		kPaletteBlue = new GFX::Palette();
		kPaletteYellow = new GFX::Palette();
		kPaletteBlack = new GFX::Palette();

		kPaletteRed->colors[0] = kTransparentColor;
		kPaletteBlue->colors[0] = kTransparentColor;
		kPaletteYellow->colors[0] = kTransparentColor;
		kPaletteBlack->colors[0] = kTransparentColor;

		for (int c = 1; c < 256; c++) {
			kPaletteRed->colors[c].r = 230;
			kPaletteRed->colors[c].g = 0;
			kPaletteRed->colors[c].b = 0;
			kPaletteRed->colors[c].a = 0;

			kPaletteYellow->colors[c].r = 155;
			kPaletteYellow->colors[c].g = 200;
			kPaletteYellow->colors[c].b = 0;
			kPaletteYellow->colors[c].a = 0;

			kPaletteBlue->colors[c].r = 0;
			kPaletteBlue->colors[c].g = 0;
			kPaletteBlue->colors[c].b = 135;
			kPaletteBlue->colors[c].a = 0;

			kPaletteBlack->colors[c].r = 0;
			kPaletteBlack->colors[c].g = 0;
			kPaletteBlack->colors[c].b = 0;
			kPaletteBlack->colors[c].a = 0;
		}
	} catch (...) {
		return false;
	}
	return true;
}


void
DestroyGlobalPalettes()
{
	delete kPaletteRed;
	delete kPaletteBlue;
	delete kPaletteYellow;
	delete kPaletteBlack;
}


}
