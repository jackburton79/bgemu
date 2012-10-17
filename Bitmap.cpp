/*
 * Bitmap.cpp
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

Bitmap::Bitmap(SDL_Surface* surface)
{
	fSurface = surface;
}


Bitmap::~Bitmap()
{
	SDL_FreeSurface(fSurface);
}


void
Bitmap::SetColors(Color* colors, int start, int num)
{
	SDL_SetColors(fSurface, (SDL_Color*)colors, start, num);
}


void
Bitmap::SetPalette(const Palette& palette)
{
	SDL_Color sdlPalette[256];
	for (uint16 c = 0; c < 256; c++) {
		sdlPalette[c].r = palette.colors[c].r;
		sdlPalette[c].g = palette.colors[c].g;
		sdlPalette[c].b = palette.colors[c].b;
		sdlPalette[c].unused = 0;
	}

	SDL_SetColors(fSurface, sdlPalette, 0, 256);
}


void
Bitmap::SetColorKey(uint32 index, bool on)
{
	SDL_SetColorKey(fSurface, on ? SDL_SRCCOLORKEY|SDL_RLEACCEL : 0, index);
}


void
Bitmap::SetColorKey(uint8 r, uint8 g, uint8 b, bool on)
{
	SDL_SetColorKey(fSurface,
			on ? SDL_SRCCOLORKEY|SDL_RLEACCEL : 0,
			SDL_MapRGB(fSurface->format, r, g, b));
}


void
Bitmap::SetAlpha(uint8 value, bool on)
{
	SDL_SetAlpha(fSurface, on ? SDL_SRCALPHA|SDL_RLEACCEL : 0, value);
}


void*
Bitmap::Pixels() const
{
	return fSurface->pixels;
}


GFX::rect
Bitmap::Frame() const
{
	GFX::rect frame = { 0, 0, fSurface->w, fSurface->h };
	return frame;
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


uint16
Bitmap::Pitch() const
{
	return fSurface->pitch;
}


SDL_Surface*
Bitmap::Surface()
{
	return fSurface;
}
