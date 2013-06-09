/*
 * Bitmap.cpp
 *
 *  Created on: 29/mag/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Utils.h"

#include <iostream>

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
Bitmap::Clear(int color)
{
	SDL_Rect rect = { 0, 0, uint16(fSurface->w), uint16(fSurface->h) };
	SDL_FillRect(fSurface, &rect, color);
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


// The following methods require the bitmap locked
void
Bitmap::PutPixel(uint16 x, uint16 y, const uint32 color)
{
	if (x >= (uint32)fSurface->w || y >= (uint32)fSurface->h)
		return;

	uint32 bpp = fSurface->format->BytesPerPixel;
	uint32 offset = fSurface->pitch * y + x * bpp;

	memcpy((uint8 *)fSurface->pixels + offset, &color, bpp);
}


void
Bitmap::StrokeLine(uint16 x1, uint16 y1,
			uint16 x2, uint16 y2, const uint32 color)
{
	int cycle;
	int lg_delta = x2 - x1;
	int sh_delta = y2 - y1;
	int lg_step = SGN(lg_delta);
	lg_delta = ABS(lg_delta);
	int sh_step = SGN(sh_delta);
	sh_delta = ABS(sh_delta);
	if (sh_delta < lg_delta) {
		cycle = lg_delta >> 1;
		while (x1 != x2) {
			PutPixel(x1, y1, color);
			cycle += sh_delta;
			if (cycle > lg_delta) {
				cycle -= lg_delta;
				y1 += sh_step;
			}
			x1 += lg_step;
		}
		PutPixel(x1, y1, color);
	}
	cycle = sh_delta >> 1;
	while (y1 != y2) {
		PutPixel(x1, y1, color);
		cycle += lg_delta;
		if (cycle > sh_delta) {
			cycle -= sh_delta;
			x1 += lg_step;
		}
		y1 += sh_step;
	}
	PutPixel(x1, y1, color);
}


void
Bitmap::StrokeRect(const GFX::rect& rect, const uint32 color)
{
	StrokeLine(rect.x, rect.y, rect.x + rect.w, rect.y, color);
	StrokeLine(rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h, color);
	StrokeLine(rect.x, rect.y, rect.x, rect.y + rect.h, color);
	StrokeLine(rect.x, rect.y + rect.h, rect.x + rect.w, rect.y + rect.h, color);
}


void
Bitmap::StrokePolygon(const Polygon& polygon,
		uint16 x, uint16 y, const uint32 newColor)
{
	const int32 numPoints = polygon.CountPoints();
	if (numPoints <= 2)
		return;

	// TODO: Use newColor
	uint32 color = SDL_MapRGB(fSurface->format, 128, 0, 30);
	const IE::point &firstPt = offset_point(polygon.PointAt(0), x, y);
	for (int32 c = 0; c < numPoints - 1; c++) {
		const IE::point &pt = offset_point(polygon.PointAt(c), x, y);
		const IE::point &nextPt = offset_point(polygon.PointAt(c + 1), x, y);
		StrokeLine(pt.x, pt.y, nextPt.x, nextPt.y, color);
		if (c == numPoints - 2)
			StrokeLine(nextPt.x, nextPt.y, firstPt.x, firstPt.y, color);
	}
}


void
Bitmap::Mirror()
{
	SDL_Surface* surface = fSurface;
	SDL_LockSurface(surface);

	for (int32 y = 0; y < surface->h; y++) {
		uint8 *sourcePixels = (uint8*)surface->pixels + y * surface->pitch;
		uint8 *destPixels = (uint8*)sourcePixels + surface->pitch - 1;
		for (int32 x = 0; x < surface->pitch / 2; x++)
			std::swap(*sourcePixels++, *destPixels--);
	}
	SDL_UnlockSurface(surface);
}


void
Bitmap::Flip()
{
	std::cerr << "Bitmap::Flip() not implemented" << std::endl;
}


bool
Bitmap::Lock()
{
	SDL_LockSurface(fSurface);
	return true;
}


void
Bitmap::Unlock()
{
	SDL_UnlockSurface(fSurface);
}


void*
Bitmap::Pixels() const
{
	return fSurface->pixels;
}


void
Bitmap::SetFromData(const void* data, uint32 width, uint32 height)
{
	SDL_LockSurface(fSurface);
	uint8 *ptr = (uint8*)data;
	uint8 *surfacePixels = (uint8*)fSurface->pixels;
	for (int32 y = 0; y < height; y++) {
		memcpy(surfacePixels, ptr + y * width, width);
		surfacePixels += fSurface->pitch;
	}
	SDL_UnlockSurface(fSurface);
}


GFX::rect
Bitmap::Frame() const
{
	GFX::rect frame = { 0, 0, uint16(fSurface->w), uint16(fSurface->h) };
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


void
Bitmap::Update()
{
	SDL_Flip(fSurface);
}


SDL_Surface*
Bitmap::Surface()
{
	return fSurface;
}


void
Bitmap::Dump()
{
	std::cout << "Bitmap " << this << std::endl;
	SDL_LockSurface(fSurface);
	for (int32 y = 0; y < fSurface->h; y++) {
		for (int32 x = 0; x < fSurface->w; x++) {
			uint8* pixel = (uint8*)fSurface->pixels + y * fSurface->pitch + x;
			std::cout << (int)*pixel << " ";
		}
		std::cout << std::endl;
	}
	SDL_UnlockSurface(fSurface);
}
