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
class Polygon;
class Bitmap : public Referenceable {
public:
	Bitmap(SDL_Surface* surface, bool ownsSurface = true);

	void Clear(int color = 0);
	void SetColors(Color* colors, int start, int num);
	void SetPalette(const Palette& palette);
	void SetColorKey(uint32 key, bool on = true);
	void SetColorKey(uint8 r, uint8 g, uint8 b, bool on = true);
	void ClearColorKey() { SetColorKey(0, false); };
	void SetAlpha(uint8 alphaValue, bool on = true);

	void PutPixel(uint16 x, uint16 y, const uint32 color);
	void StrokeLine(uint16 xStart, uint16 yStart,
			uint16 xEnd, uint16 yEnd, const uint32 color);
	void StrokeRect(const GFX::rect& rect, const uint32 color);
	void StrokePolygon(const Polygon& poly,
			uint16 x, uint16 y, const uint32 color);

	void Mirror();
	void Flip();

	bool Lock();
	void Unlock();
	void* Pixels() const;
	void SetFromData(const void *data, uint32 width, uint32 height);

	GFX::rect Frame() const;
	uint16 Width() const;
	uint16 Height() const;
	uint16 Pitch() const;

	void Update();

	SDL_Surface* Surface();

	void Dump();
	void Save(const char* fileName);

private:
	friend class GraphicsEngine;

	SDL_Surface* fSurface;
	bool fOwnsSurface;

	Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel);
	~Bitmap();
};

#endif /* SPRITE_H_ */
