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

#include <algorithm>
#include <iostream>
#include <vector>

#include "SDL.h"

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

}


Bitmap::Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel)
	:
	fMirrored(NULL),
	fXOffset(0),
	fYOffset(0),
	fOwnsSurface(true)
{
	fSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			bytesPerPixel, 0, 0, 0, 0);

}


Bitmap::Bitmap(SDL_Surface* surface, bool ownsSurface)
	:
	fSurface(surface),
	fMirrored(NULL),
	fXOffset(0),
	fYOffset(0),
	fOwnsSurface(ownsSurface)
{
}


Bitmap::~Bitmap()
{
	if (fOwnsSurface)
		SDL_FreeSurface(fSurface);
	delete fMirrored;
}


void
Bitmap::Clear(uint32 color)
{
	SDL_Rect rect = { 0, 0, uint16(fSurface->w), uint16(fSurface->h) };
	SDL_FillRect(fSurface, &rect, color);
}


void
Bitmap::SetColors(Color* colors, uint8 start, int num)
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
		sdlPalette[c].unused = palette.colors[c].a;
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
	if (x >= (uint16)fSurface->w || y >= (uint16)fSurface->h)
		return;

	uint32 bytesPerPixel = fSurface->format->BytesPerPixel;
	uint32 offset = fSurface->pitch * y + x * bytesPerPixel;

	memcpy((uint8*)fSurface->pixels + offset, &color, bytesPerPixel);
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
Bitmap::StrokePolygon(const Polygon& polygon, const uint32 color)
{
	const int32 numPoints = polygon.CountPoints();
	if (numPoints <= 2)
		return;

	for (int32 c = 0; c < numPoints; c++) {
		const IE::point &pt = polygon.PointAt(c);
		const IE::point &nextPt = (c == numPoints - 1) ?
				polygon.PointAt(0) : polygon.PointAt(c + 1);

		// TODO: Why does this happen ?
		// If we don't do this, the negative points become positive inside
		// StrokeLine, since it does accept unsigned integers
		if (pt.x < 0 || pt.y < 0 || nextPt.x < 0 || nextPt.y < 0)
			continue;

		StrokeLine(pt.x, pt.y, nextPt.x, nextPt.y, color);
	}
}


void
Bitmap::FillPolygon(const Polygon& polygon, const uint32 color)
{
	const int32 numPoints = polygon.CountPoints();
	const sint16 top = std::max(polygon.Frame().y, sint16(0));
	const sint16 bottom = std::min(top + polygon.Frame().h, int(Frame().h));

	for (sint16 y = top; y < bottom; y++) {
		std::vector<sint16> nodeList;
		for (int32 p = 0; p < numPoints; p++) {
			const IE::point& pointA = polygon.PointAt(p);
			const IE::point& pointB = (p == numPoints - 1) ?
					polygon.PointAt(0) : polygon.PointAt(p + 1);

			if ((pointA.y < y && pointB.y >= y)
					|| (pointA.y >= y && pointB.y < y)) {
				nodeList.push_back(pointA.x + (y - pointA.y)
						* (pointB.x - pointA.x) / (pointB.y - pointA.y));
			}
		}

		if (nodeList.size() > 1) {
			std::sort(nodeList.begin(), nodeList.end());
			for (size_t c = 0; c < nodeList.size() - 1; c+=2) {
				IE::point ptStart = { int16(nodeList[c]), y };
				IE::point ptEnd = { int16(nodeList[c + 1]), y };

				StrokeLine(std::max(ptStart.x, sint16(0)), ptStart.y,
						std::max(ptEnd.x, sint16(0)), ptEnd.y, color);
			}
			nodeList.clear();
		}
	}
}
// end


Bitmap*
Bitmap::GetMirrored() const
{
	if (fMirrored == NULL) {
		const_cast<Bitmap*>(this)->fMirrored = Clone();
		fMirrored->_Mirror();
	}

	return fMirrored;
}


void
Bitmap::Flip()
{
	std::cerr << "Bitmap::Flip() not implemented" << std::endl;
}


void
Bitmap::SetPosition(uint16 x, uint16 y)
{
	fXOffset = x;
	fYOffset = y;
}


bool
Bitmap::Lock() const
{
	SDL_LockSurface(fSurface);
	return true;
}


void
Bitmap::Unlock() const
{
	SDL_UnlockSurface(fSurface);
}


void*
Bitmap::Pixels() const
{
	return fSurface->pixels;
}


void
Bitmap::ImportData(const void* data, uint32 width, uint32 height)
{
	SDL_LockSurface(fSurface);
	uint8 *ptr = (uint8*)data;
	uint8 *surfacePixels = (uint8*)fSurface->pixels;
	for (uint32 y = 0; y < height; y++) {
		memcpy(surfacePixels, ptr + y * width, width);
		surfacePixels += fSurface->pitch;
	}
	SDL_UnlockSurface(fSurface);
}


GFX::rect
Bitmap::Frame() const
{
	return GFX::rect(fXOffset, fYOffset, uint16(fSurface->w),
			uint16(fSurface->h));
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


Bitmap*
Bitmap::Clone() const
{
	// Makes a copy of the original surface
	SDL_Surface* surface = SDL_ConvertSurface(fSurface,
							fSurface->format, SDL_SWSURFACE);
	Bitmap* newBitmap = new Bitmap(surface, true);
	return newBitmap;
}


SDL_Surface*
Bitmap::Surface() const
{
	return fSurface;
}


void
Bitmap::Dump() const
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


/* static */
Bitmap*
Bitmap::Load(const char* fileName)
{
	SDL_Surface *surface = SDL_LoadBMP(fileName);
	if (surface == NULL)
		return NULL;

	return new Bitmap(surface);
}


/* static */
Bitmap*
Bitmap::Load(const void* data, const size_t size)
{
	SDL_RWops *rw = SDL_RWFromMem((void*)data, size);
	if (rw == NULL)
		return NULL;

	SDL_Surface *surface = SDL_LoadBMP_RW(rw, 1);
	if (surface == NULL)
		return NULL;

	return new Bitmap(surface);
}


void
Bitmap::Save(const char* fileName) const
{
	SDL_SaveBMP(fSurface, fileName);
}


void
Bitmap::_Mirror()
{
	SDL_LockSurface(fSurface);

	for (int32 y = 0; y < fSurface->h; y++) {
		uint8 *sourcePixels = (uint8*)fSurface->pixels + y * fSurface->pitch;
		uint8 *destPixels = (uint8*)sourcePixels + fSurface->pitch - 1;
		for (int32 x = 0; x < fSurface->pitch / 2; x++)
			std::swap(*sourcePixels++, *destPixels--);
	}
	SDL_UnlockSurface(fSurface);

	SetPosition(Frame().x - Width(), Frame().y);
}
