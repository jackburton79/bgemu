/*
 * Sprite.cpp
 *
 *  Created on: 29/mag/2012
 *      Author: stefano
 */

#include "Bitmap.h"

Bitmap::Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel)
{
	fSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			bytesPerPixel, 0, 0, 0, 0);

}

Bitmap::~Bitmap()
{
	SDL_FreeSurface(fSurface);
}


void
Bitmap::SetPalette(const Palette& palette)
{
	SDL_Color sdlPalette[256];
	for (uint16 c = 0; c < 256; c++) {
		sdlPalette[c].r = palette.colors[c].r;
		sdlPalette[c].g = palette.colors[c].g;
		sdlPalette[c].b = palette.colors[c].b;
	}

	SDL_SetColors(fSurface, sdlPalette, 0, 256);
}


void
Bitmap::SetColorKey(uint8 index)
{
	SDL_SetColorKey(fSurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, index);
}


void*
Bitmap::Pixels() const
{
	return fSurface->pixels;
}


uint16
Bitmap::Width() const
{
	return fSurface->w;
}


uint16
Bitmap::Height() const
{
	return fSurface->h;
}


SDL_Surface*
Bitmap::Surface()
{
	return fSurface;
}
