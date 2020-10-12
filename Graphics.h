#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include "SupportDefs.h"

class Bitmap;
class Graphics {
public:
	static int DecodeRLE(const void* src, uint32 size, void* dst, uint8 cmpIndex);
	static void ApplyShade(Bitmap*);
	static Bitmap* ApplyMask(Bitmap* surface, Bitmap* mask, uint16 x, uint16 y);
};

#endif
