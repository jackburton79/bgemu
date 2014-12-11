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
Bitmap::GetPalette(Palette& palette) const
{
	SDL_Color* sdlPalette = fSurface->format->palette->colors;

	for (uint16 c = 0; c < 256; c++) {
		palette.colors[c].r = sdlPalette[c].r;
		palette.colors[c].g = sdlPalette[c].g;
		palette.colors[c].b = sdlPalette[c].b;
		palette.colors[c].a = sdlPalette[c].unused;
	}
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
Bitmap::PutPixel(int32 x, int32 y, const uint32 color)
{
	if (x < 0 || y < 0 || x >= (uint16)fSurface->w || y >= (uint16)fSurface->h)
		return;

	uint32 bytesPerPixel = fSurface->format->BytesPerPixel;
	uint32 offset = fSurface->pitch * y + x * bytesPerPixel;

	memcpy((uint8*)fSurface->pixels + offset, &color, bytesPerPixel);
}


void
Bitmap::StrokeLine(int32 x1, int32 y1,
			int32 x2, int32 y2, const uint32 color)
{
	int cycle;
	int deltaX = x2 - x1;
	int deltaY = y2 - y1;
	int stepX = SGN(deltaX);
	deltaX = ABS(deltaX);
	int stepY = SGN(deltaY);
	deltaY = ABS(deltaY);
	if (deltaY < deltaX) {
		cycle = deltaX >> 1;
		while (x1 != x2) {
			PutPixel(x1, y1, color);
			cycle += deltaY;
			if (cycle > deltaX) {
				cycle -= deltaX;
				y1 += stepY;
			}
			x1 += stepX;
		}
		PutPixel(x1, y1, color);
	}
	cycle = deltaY >> 1;
	while (y1 != y2) {
		PutPixel(x1, y1, color);
		cycle += deltaX;
		if (cycle > deltaY) {
			cycle -= deltaY;
			x1 += stepX;
		}
		y1 += stepY;
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
Bitmap::FillRect(const GFX::rect& rect, const uint32 color)
{
	SDL_FillRect(fSurface, (SDL_Rect*)&rect, (Uint8)color);
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

		StrokeLine(pt.x, pt.y, nextPt.x, nextPt.y, color);
	}
}


void
Bitmap::StrokePolygon(const Polygon& polygon, const uint32 color,
						int32 xOffset, int32 yOffset)
{
	Polygon newPolygon = polygon;
	newPolygon.OffsetBy(xOffset, yOffset);
	StrokePolygon(newPolygon, color);
}


void
Bitmap::FillPolygon(const Polygon& polygon, const uint32 color)
{
	const int32 numPoints = polygon.CountPoints();
	const sint16 bottom = polygon.Frame().y + polygon.Frame().h;
	const sint16 top = std::max(polygon.Frame().y, sint16(0));

	for (uint16 y = top; y < bottom; y++) {
		std::vector<int32> nodeList;
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
			for (size_t c = 0; c < nodeList.size(); c+=2) {
				StrokeLine(nodeList[c], y, nodeList[c + 1], y, color);
			}
		}
	}
}


void
Bitmap::FillPolygon(const Polygon& polygon, const uint32 color,
					int32 xOffset, int32 yOffset)
{
	Polygon newPolygon = polygon;
	newPolygon.OffsetBy(xOffset, yOffset);
	FillPolygon(newPolygon, color);
}


static void
PutCirclePixels(Bitmap* bitmap, const int centerX, const int centerY,
		const int x, const int y, const uint32 color)
{
	bitmap->PutPixel(centerX + x, centerY + y, color);
	bitmap->PutPixel(centerX + x, centerY - y, color);
	bitmap->PutPixel(centerX - x, centerY + y, color);
	bitmap->PutPixel(centerX - x, centerY - y, color);
	bitmap->PutPixel(centerX + y, centerY + x, color);
	bitmap->PutPixel(centerX + y, centerY - x, color);
	bitmap->PutPixel(centerX - y, centerY + x, color);
	bitmap->PutPixel(centerX - y, centerY - x, color);
}


void
Bitmap::StrokeCircle(const int16& centerX, const int16& centerY, const uint32 radius,
		const uint32 color)
{
	uint32 x = 0;
	uint32 y = radius;
	int decision = 3 - 2 * radius;
	while (x <= y) {
		PutCirclePixels(this, centerX, centerY, x, y, color);
		if (decision < 0)
			decision = decision + (4 * x) + 6;
		else {
			decision = decision + 4 * (x - y) + 10;
			y = y - 1;
		}
		x++;
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
	return GFX::rect(fXOffset, fYOffset, Width(), Height());
}


uint16
Bitmap::Width() const
{
	return (uint16)fSurface->w;
}


uint16
Bitmap::Height() const
{
	return (uint16)fSurface->h;
}


uint16
Bitmap::Pitch() const
{
	return fSurface->pitch;
}


uint16
Bitmap::BitsPerPixel() const
{
	return fSurface->format->BitsPerPixel;
}



uint32
Bitmap::MapColor(const uint8 r, const uint8 g, const uint8 b)
{
	return SDL_MapRGB(fSurface->format, r, g, b);
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
	newBitmap->fXOffset = fXOffset;
	newBitmap->fYOffset = fYOffset;
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

	uint8* sourcePixels = (uint8*)fSurface->pixels;
	for (int y = 0; y < fSurface->h; y++) {
		uint8* destPtr = (uint8*)sourcePixels + fSurface->w - 1;
		uint8* sourcePtr = sourcePixels;
		while (sourcePtr < destPtr)
			std::swap(*sourcePtr++, *destPtr--);
		sourcePixels += fSurface->pitch;
	}
	SDL_UnlockSurface(fSurface);

	uint16 newX = std::max(Frame().x - Width(), 0);
	SetPosition(newX, fYOffset);
}
