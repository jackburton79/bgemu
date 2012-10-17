/*
 * Sprite.h
 *
 *  Created on: 29/mag/2012
 *      Author: stefano
 */

#ifndef SPRITE_H_
#define SPRITE_H_

#include "Referenceable.h"
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

namespace GFX {

struct rect {
	sint16 x;
	sint16 y;
	uint16 w;
	uint16 h;
};

}

class GraphicsEngine;
class Bitmap : public Referenceable {
public:
	Bitmap(SDL_Surface* surface);

	void SetColors(Color* colors, int start, int num);
	void SetPalette(const Palette& palette);
	void SetColorKey(uint32 key, bool on = true);
	void SetColorKey(uint8 r, uint8 g, uint8 b, bool on = true);
	void ClearColorKey();
	void SetAlpha(uint8 alphaValue, bool on = true);

	void* Pixels() const;

	GFX::rect Frame() const;
	uint16 Width() const;
	uint16 Height() const;
	uint16 Pitch() const;

	SDL_Surface* Surface();
private:
	friend class GraphicsEngine;

	SDL_Surface* fSurface;

	Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel);
	~Bitmap();
};

#endif /* SPRITE_H_ */
