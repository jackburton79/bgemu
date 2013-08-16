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

		// TODO: Why does this happen ?
		// If we don't do this, the negative points become positive inside
		// StrokeLine, since it does accept unsigned integers
		if (pt.x < 0 || pt.y < 0 || nextPt.x < 0 || nextPt.y < 0)
			continue;

		StrokeLine(pt.x, pt.y, nextPt.x, nextPt.y, color);
		if (c == numPoints - 2)
			StrokeLine(nextPt.x, nextPt.y, firstPt.x, firstPt.y, color);
	}
}


void
Bitmap::FillPolygon(const Polygon& polygon,
		uint16 xOffset, uint16 yOffset, const uint32 color)
{
	if (polygon.IsHole())
		return;

	const sint16 top = polygon.Frame().y;
	const sint16 bottom = polygon.Frame().y + polygon.Frame().h;

	for (sint16 y = top; y < bottom; y++) {
		std::vector<sint16> nodeList;
		if (polygon.IsHole())
			nodeList.push_back(polygon.Frame().x);
		for (int32 p = 0; p < polygon.CountPoints() - 1; p++) {
			const IE::point& pointA = polygon.PointAt(p);
			const IE::point& pointB = polygon.PointAt(p + 1);

			if ((pointA.y < y && pointB.y >= y)
					|| (pointA.y >= y && pointB.y < y)) {
				nodeList.push_back(pointA.x + (y - pointA.y)
						/ (pointB.y - pointA.y) * (pointB.x - pointA.x));
			}
		}
		if (polygon.IsHole())
			nodeList.push_back(polygon.Frame().x + polygon.Frame().w);

		std::sort(nodeList.begin(), nodeList.end());

		if (nodeList.size() > 1) {
			for (size_t c = 0; c < nodeList.size() - 1; c+=2) {
				sint16 xStart = nodeList[c];
				sint16 xEnd = nodeList[c + 1];
				IE::point ptStart = { xStart, y };
				IE::point ptEnd = { xEnd, y };
				ptStart = offset_point(ptStart, xOffset, yOffset);
				ptEnd = offset_point(ptEnd, xOffset, yOffset);

				// TODO: Why does this happen ?
				// If we don't do this, the negative points become positive inside
				// StrokeLine, since it does accept unsigned integers
				StrokeLine(std::max(ptStart.x, (sint16)0),
						std::max(ptStart.y, (sint16)0),
						std::max(ptEnd.x, (sint16)0),
						std::max(ptEnd.y, (sint16)0),
						color);
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
	GFX::rect frame = {
			fXOffset,
			fYOffset,
			uint16(fSurface->w),
			uint16(fSurface->h)
	};
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


Bitmap*
Bitmap::Clone() const
{
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



void
Bitmap::_Mirror()
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

	SetPosition(Frame().x - Width(), Frame().y);
}
