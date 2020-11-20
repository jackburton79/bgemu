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



Palette::Palette(const GFX::Color& start, const GFX::Color& end)
{
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


void
Palette::ModColor(uint8 index, uint8 mod)
{
	return;
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


}


