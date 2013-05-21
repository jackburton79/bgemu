#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <SDL.h>

#include "SupportDefs.h"

class Bitmap;
class Polygon;
namespace GFX {
	struct rect;
}
class Graphics {
public:
	static int DecodeRLE(const void *src, uint32 size, void *dst, uint8 cmpIndex);
	static int DataToBitmap(const void *data, int32 width, int32 height,
			Bitmap *surface);

	static SDL_Surface *MirrorSDLSurface(SDL_Surface *surface);
	static Bitmap* ApplyMask(Bitmap* surface, Bitmap* mask, uint16 x, uint16 y);

	static void DrawPixel(SDL_Surface *surface, uint32 x, uint32 y, uint32 color);
	static void DrawLine(SDL_Surface *surface, uint32 x1, uint32 y1,
				uint32 x2, uint32 y2, uint32 color);
	static void DrawPolygon(Polygon &polygon, SDL_Surface *surface, uint16 x, uint16 y);
	static void DrawRect(SDL_Surface* surface, SDL_Rect& rect, uint32 color);
	static void DrawRect(Bitmap* bitmap, GFX::rect& rect, uint32 color);
};

#endif
