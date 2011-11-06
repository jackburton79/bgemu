#ifndef __FRAME_H
#define __FRAME_H

#include "SDL.h"

struct TileCell {
	SDL_Surface *surface;
	SDL_Rect rect;
};

typedef TileCell Frame;

#endif
