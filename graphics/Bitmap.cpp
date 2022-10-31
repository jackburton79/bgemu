/*
 * Bitmap.cpp
 *
 *  Created on: 29/mag/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "GraphicsEngine.h"
#include "Log.h"
#include "Polygon.h"
#include "RectUtils.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "SDL.h"


Bitmap::Bitmap(uint16 width, uint16 height, uint16 bitsPerPixel)
	:
	AutoDeletingReferenceable(),
	fMirrored(NULL),
	fOffset(GFX::kOrigin),
	fOwnsSurface(true)
{
	fSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height,
			bitsPerPixel, 0, 0, 0, 0);
}



Bitmap::Bitmap(SDL_Surface* surface, bool ownsSurface)
	:
	AutoDeletingReferenceable(),
	fSurface(surface),
	fMirrored(NULL),
	fOffset(GFX::kOrigin),
	fOwnsSurface(ownsSurface)
{
}


/* virtual */
Bitmap::~Bitmap()
{
	if (fOwnsSurface)
		SDL_FreeSurface(fSurface);
	delete fMirrored;
}


void
Bitmap::Clear(uint32 color)
{
	SDL_Rect rect = { 0, 0, fSurface->w, fSurface->h };
	SDL_FillRect(fSurface, &rect, color);
}


void
Bitmap::SetColors(GFX::Color* colors, uint8 start, int num)
{
	SDL_SetPaletteColors(fSurface->format->palette, (SDL_Color*)colors, start, num);
}


void
Bitmap::SetColors(const GFX::Color& color, uint8 start, int num)
{
	struct GFX::Color colorList[num];
	for (int i = 0; i < num; i++) {
		colorList[i] = color;
	}
	SDL_SetPaletteColors(fSurface->format->palette, (SDL_Color*)colorList, start, num);
}


void
Bitmap::GetPalette(GFX::Palette& palette) const
{
	SDL_Color* sdlPalette = fSurface->format->palette->colors;

	for (uint16 c = 0; c < 256; c++) {
		palette.colors[c].r = sdlPalette[c].r;
		palette.colors[c].g = sdlPalette[c].g;
		palette.colors[c].b = sdlPalette[c].b;
		palette.colors[c].a = sdlPalette[c].a;
	}
}


void
Bitmap::SetPalette(const GFX::Palette& palette)
{
	SDL_Color sdlPalette[256];
	for (uint16 c = 0; c < 256; c++) {
		sdlPalette[c].r = palette.colors[c].r;
		sdlPalette[c].g = palette.colors[c].g;
		sdlPalette[c].b = palette.colors[c].b;
		sdlPalette[c].a = palette.colors[c].a;
	}

	SDL_SetPaletteColors(fSurface->format->palette, sdlPalette, 0, 256);
}


bool
Bitmap::GetColorKey(uint32& colorKey) const
{
	return SDL_GetColorKey(fSurface, &colorKey) == 0;
}


void
Bitmap::SetColorKey(uint32 index, bool on)
{
	SDL_SetColorKey(fSurface, on ? SDL_TRUE|SDL_RLEACCEL : 0, index);
}


void
Bitmap::SetColorKey(uint8 r, uint8 g, uint8 b, bool on)
{
	SDL_SetColorKey(fSurface,
			on ? SDL_TRUE|SDL_RLEACCEL : 0,
			SDL_MapRGB(fSurface->format, r, g, b));
}


void
Bitmap::SetAlpha(uint8 value, bool on)
{
	if (value == 255) {
		std::cerr << RED("BUG! SetAlpha(255) fails. Use SetAlpha(254)!") << std::endl;
		value = 254;		
	}

	SDL_SetSurfaceAlphaMod(fSurface, value);
	if (on)
		SDL_SetSurfaceBlendMode(fSurface, SDL_BLENDMODE_BLEND);
	//else
		//SDL_SetSurfaceBlendMode(fSurface, SDL_BLENDMODE_NONE);
}


// The following methods require the bitmap locked
uint32
Bitmap::GetPixel(uint16 x, uint16 y) const
{
    int bpp = fSurface->format->BytesPerPixel;
    Uint8 *p = (Uint8*)fSurface->pixels + y * fSurface->pitch + x * bpp;

	switch(bpp) {
		case 1:
			return *p;
			break;
		case 2:
			return *(Uint16*)p;
			break;
		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
				return p[0] << 16 | p[1] << 8 | p[2];
			else
				return p[0] | p[1] << 8 | p[2] << 16;
			break;
		case 4:
			return *(Uint32*)p;
			break;
		default:
			return 0;
	}
}


void
Bitmap::PutPixel(int32 x, int32 y, const uint32 color)
{
	if (x < 0 || y < 0 || x >= fSurface->w || y >= fSurface->h)
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
	int stepX = std::signbit(deltaX) ? -1 : 1;
	deltaX = std::abs(deltaX);
	int stepY = std::signbit(deltaY) ? -1 : 1;
	deltaY = std::abs(deltaY);
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
	SDL_Rect sdlRect;
	GFXRectToSDLRect(&rect, &sdlRect);
	SDL_FillRect(fSurface, &sdlRect, (Uint8)color);
}


void
Bitmap::StrokePolygon(const Polygon& polygon, const uint32 color)
{
	const int32 numPoints = polygon.CountPoints();
	if (numPoints <= 2)
		return;

	for (int32 c = 0; c < numPoints; c++) {
		const GFX::point &pt = polygon.PointAt(c);
		const GFX::point &nextPt = (c == numPoints - 1) ?
				polygon.PointAt(0) : polygon.PointAt(c + 1);

		StrokeLine(pt.x, pt.y, nextPt.x, nextPt.y, color);
	}
}


void
Bitmap::StrokePolygon(const Polygon& polygon, const uint32 color,
						int32 xOffset, int32 yOffset)
{
	if (polygon.CountPoints() <= 2)
		return;

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
			const GFX::point& pointA = polygon.PointAt(p);
			const GFX::point& pointB = (p == numPoints - 1) ?
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


// TODO: Quick, lazy and unoptimized implementation
void
Bitmap::FillCircle(const int16& centerX, const int16& centerY, const uint32 radius,
		const uint32 color)
{
	for (uint32 r = 1; r < radius; r++) {
		StrokeCircle(centerX, centerY, r, color);
	}
}

// end


void
Bitmap::BlitTo(Bitmap* target, const GFX::point& where) const
{
	GFX::rect sourceRect = { 0, 0, Width(), Height() };
	GFX::rect destRect = offset_rect(sourceRect, where.x, where.y);
	GraphicsEngine::BlitBitmap(this, &sourceRect, target, &destRect);
}


Bitmap*
Bitmap::GetMirrored() const
{
	if (fMirrored == NULL) {
		const_cast<Bitmap*>(this)->fMirrored = Clone();
		fMirrored->Mirror();
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
	fOffset.x = x;
	fOffset.y = y;
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
	return GFX::rect(fOffset.x, fOffset.y, Width(), Height());
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


Bitmap*
Bitmap::Clone() const
{
	// Makes a copy of the original surface
	SDL_Surface* surface = SDL_ConvertSurface(fSurface,
							fSurface->format, SDL_SWSURFACE);
	
	Bitmap* newBitmap = new Bitmap(surface, true);
	
	newBitmap->fOffset = fOffset;

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
Bitmap::Mirror()
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
	SetPosition(newX, fOffset.y);
}


// DataBitmap
DataBitmap::DataBitmap(void* data, uint16 width, uint16 height, uint16 depth, bool ownsData)
	:
	Bitmap(SDL_CreateRGBSurfaceFrom(data, width,
							height, depth, width, 0, 0, 0, 0), true),
	fData((uint8*)data),
	fOwns(ownsData)
{
	// SDL_CreateRGBSurfaceFrom doesn't free the passed data,
	// that's why we have to create a custom Bitmap class
}


DataBitmap::~DataBitmap()
{
	if (fOwns)
		delete[] fData;
}
