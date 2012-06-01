#ifndef __FRAME_H
#define __FRAME_H

#include "Bitmap.h"

struct Frame {
	Bitmap *bitmap;
	GFX::rect rect;
};

#endif // __FRAME_H
