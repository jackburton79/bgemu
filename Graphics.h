#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <SDL.h>
#include <SupportDefs.h>

class Polygon;
class Graphics {
public:
	static int DecodeRLE(const void *src, uint32 size, void *dst, uint8 cmpIndex);
	static int Decode(const void *source, uint32 outSize, void *dest);
	static int DataToSDLSurface(const void *data, int32 width, int32 height,
			SDL_Surface *surface);
	static void DrawPixel(SDL_Surface *surface, uint32 x, uint32 y, uint32 color);
	static void DrawLine(SDL_Surface *surface, uint32 x1, uint32 y1,
				uint32 x2, uint32 y2, uint32 color);
	static void DrawPolygon(Polygon &polygon, SDL_Surface *surface);
};

#endif
