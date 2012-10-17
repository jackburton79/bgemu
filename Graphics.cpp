#include "Graphics.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <string.h>

/* static */
int
Graphics::DecodeRLE(const void *source, uint32 outSize,
		void *dest, uint8 compIndex)
{
	uint32 size = 0;
	uint8 *bits = (uint8*)dest;
	uint8 *srcBits = (uint8*)source;
	while (size++ < outSize) {
		uint8 byte = *srcBits++;
		if (byte == compIndex) {
			uint16 howMany = (uint8)*srcBits++;
			size += howMany;
			howMany++;
			memset(bits, byte, howMany);
			bits += howMany;
		} else {
			*bits++ = byte;
		}
	}

	size--;
	assert (size == outSize);
	return outSize;
}


/* static */
int
Graphics::Decode(const void *source, uint32 outSize, void *dest)
{
	memcpy(dest, source, outSize);
	return outSize;
}


/* static */
void
Graphics::DrawPixel(SDL_Surface *surface, uint32 x, uint32 y, uint32 color)
{
	uint32 bpp = surface->format->BytesPerPixel;
	uint32 offset = surface->pitch * y + x * bpp;

	SDL_LockSurface(surface);
	memcpy((uint8 *)surface->pixels + offset, &color, bpp);
	SDL_UnlockSurface(surface);
}


/* static */
void
Graphics::DrawLine(SDL_Surface *surface, uint32 x1, uint32 y1, uint32 x2,
		uint32 y2, uint32 color)
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
			DrawPixel(surface, x1, y1, color);
			cycle += sh_delta;
			if (cycle > lg_delta) {
				cycle -= lg_delta;
				y1 += sh_step;
			}
			x1 += lg_step;
		}
		DrawPixel(surface, x1, y1, color);
	}
	cycle = sh_delta >> 1;
	while (y1 != y2) {
		DrawPixel(surface, x1, y1, color);
		cycle += lg_delta;
		if (cycle > sh_delta) {
			cycle -= sh_delta;
			x1 += lg_step;
		}
		y1 += sh_step;
	}
	DrawPixel(surface, x1, y1, color);
}


/* static */
void
Graphics::DrawPolygon(Polygon &polygon, SDL_Surface *surface, uint16 x, uint16 y)
{
	const int32 numPoints = polygon.CountPoints();
	if (numPoints <= 2)
		return;

	uint32 color = SDL_MapRGB(surface->format, 128, 0, 30);
	const IE::point &firstPt = offset_point(polygon.PointAt(0), x, y);
	for (int32 c = 0; c < numPoints - 1; c++) {
		const IE::point &pt = offset_point(polygon.PointAt(c), x, y);
		const IE::point &nextPt = offset_point(polygon.PointAt(c + 1), x, y);
		DrawLine(surface, pt.x, pt.y, nextPt.x, nextPt.y, color);
		if (c == numPoints - 2)
			DrawLine(surface, nextPt.x, nextPt.y, firstPt.x, firstPt.y, color);
	}
}


/* static */
void
Graphics::DrawRect(SDL_Surface* surface, SDL_Rect& rect, uint32 color)
{
	DrawLine(surface, rect.x, rect.y, rect.x + rect.w, rect.y, color);
	DrawLine(surface, rect.x + rect.w, rect.y, rect.x + rect.w, rect.y + rect.h, color);
	DrawLine(surface, rect.x, rect.y, rect.x, rect.y + rect.h, color);
	DrawLine(surface, rect.x, rect.y + rect.h, rect.x + rect.w, rect.y + rect.h, color);
}


/* static */
int
Graphics::DataToBitmap(const void *data, int32 width, int32 height, Bitmap *bitmap)
{
	SDL_Surface* surface = bitmap->Surface();
	SDL_LockSurface(surface);
	uint8 *ptr = (uint8*)data;
	uint8 *surfacePixels = (uint8*)surface->pixels;
	for (int32 y = 0; y < height; y++) {
		memcpy(surfacePixels, ptr + y * width, width);
		surfacePixels += surface->pitch;
	}
	SDL_UnlockSurface(surface);
	return 0;
}


/* static */
SDL_Surface *
Graphics::MirrorSDLSurface(SDL_Surface *surface)
{
	SDL_LockSurface(surface);

	for (int32 y = 0; y < surface->h; y++) {
		uint8 *sourcePixels = (uint8*)surface->pixels + y * surface->pitch;
		uint8 *destPixels = (uint8*)sourcePixels + surface->pitch - 1;
		for (int32 x = 0; x < surface->pitch / 2; x++)
			std::swap(*sourcePixels++, *destPixels--);
	}
	SDL_UnlockSurface(surface);

	return surface;
}


/* static */
Bitmap*
Graphics::ApplyMask(Bitmap* bitmap, Bitmap* mask, uint16 x, uint16 y)
{
	SDL_Surface* surface = bitmap->Surface();
	uint32 color = SDL_MapRGB(surface->format, 0, 0, 0);
	uint32 colorKey = SDL_MapRGB(surface->format, 0, 255, 0);

	SDL_SetColorKey(surface, 0, 0);
	SDL_LockSurface(surface);
	for (int32 y = 0; y < surface->h; y++) {
		uint8 *pixels = (uint8*)surface->pixels + y * surface->pitch;
		for (int32 x = 0; x < surface->pitch; x++) {
			if (*pixels == color)
				*pixels = colorKey;
			pixels++;
		}
	}
	SDL_UnlockSurface(surface);
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, colorKey);


	return bitmap;
}


static int
IndexOfColor(const SDL_Color *color, const SDL_Palette *palette)
{
	for (int32 i = 0; i < palette->ncolors; i++) {
		if (color->r == palette->colors[i].r
			&& color->g == palette->colors[i].g
			&& color->b == palette->colors[i].b) {
			return i;
		}
	}
	return -1;
}



