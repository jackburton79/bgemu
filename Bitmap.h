/*
 * Sprite.h
 *
 *  Created on: 29/mag/2012
 *      Author: stefano
 */

#ifndef SPRITE_H_
#define SPRITE_H_

#include "SupportDefs.h"

#include "SDL.h"

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};

struct Palette {
	Color colors[256];
};


class Bitmap {
public:
	Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel);
	~Bitmap();

	void SetPalette(const Palette& palette);
	void SetColorKey(uint8 index);

	void* Pixels() const;

	uint16 Width() const;
	uint16 Height() const;

	SDL_Surface* Surface();
private:
	SDL_Surface* fSurface;
};

#endif /* SPRITE_H_ */
