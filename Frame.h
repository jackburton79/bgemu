#ifndef __FRAME_H
#define __FRAME_H

#include "SDL.h"

struct Frame {
	SDL_Surface *surface;
	SDL_Rect rect;
};

#endif // __FRAME_H
