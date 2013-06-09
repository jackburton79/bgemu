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

	static Bitmap* ApplyMask(Bitmap* surface, Bitmap* mask, uint16 x, uint16 y);
};

#endif
