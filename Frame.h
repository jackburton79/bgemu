#ifndef __FRAME_H
#define __FRAME_H

#include "SDL.h"

class Bitmap;
struct Frame {
	Bitmap *bitmap;
	SDL_Rect rect;
};

#endif // __FRAME_H
