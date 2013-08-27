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

struct SDL_Surface;

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

	rect();
	rect(sint16 x, sint16 y, uint16 width, uint16 height);
	void Print() const;
};

};

class GraphicsEngine;
class Polygon;
class Bitmap : public Referenceable {
public:

	void ImportData(const void *data, uint32 width, uint32 height);
	void* Pixels() const;

	GFX::rect Frame() const;
	uint16 Width() const;
	uint16 Height() const;
	uint16 Pitch() const;
	uint16 BitsPerPixel() const;

	void SetPosition(uint16 x, uint16 y);

	void Clear(uint32 color = 0);
	void SetColors(Color* colors, uint8 start, int num);
	void SetPalette(const Palette& palette);
	void SetColorKey(uint32 key, bool on = true);
	void SetColorKey(uint8 r, uint8 g, uint8 b, bool on = true);
	void ClearColorKey() { SetColorKey(0, false); };
	void SetAlpha(uint8 alphaValue, bool on = true);

	void PutPixel(int32 x, int32 y, const uint32 color);
	void StrokeLine(int32 xStart, int32 yStart,
			int32 xEnd, int32 yEnd, const uint32 color);
	void StrokeRect(const GFX::rect& rect, const uint32 color);
	void StrokePolygon(const Polygon& polygon, const uint32 color);
	void FillPolygon(const Polygon& polygon, const uint32 color);
	void StrokeCircle(const int16& centerX, const int16& centerY,
					const uint32 radius, const uint32 color);

	uint32 MapColor(const uint8 r, const uint8 g, const uint8 b);

	Bitmap* GetMirrored() const;
	void Flip();

	bool Lock() const;
	void Unlock() const;

	void Update();

	Bitmap* Clone() const;

	static Bitmap* Load(const char* fileName);
	static Bitmap* Load(const void* data, const size_t size);

	void Save(const char* fileName) const;
	void Dump() const;

private:
	friend class GraphicsEngine;

	SDL_Surface* fSurface;
	Bitmap* fMirrored;

	uint16 fXOffset;
	uint16 fYOffset;
	bool fOwnsSurface;

	Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel);
	Bitmap(SDL_Surface* surface, bool ownsSurface = true);

	~Bitmap();

	SDL_Surface* Surface() const;

	void _Mirror();
};

#endif /* SPRITE_H_ */
