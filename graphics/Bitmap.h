/*
 * Bitmap.h
 *
 *  Created on: 29/mag/2012
 *      Author: Stefano Ceccherini
 */

#ifndef BITMAP_H_
#define BITMAP_H_

#include "Referenceable.h"
#include "GraphicsDefs.h"
#include "SupportDefs.h"

struct SDL_Surface;


class GraphicsEngine;
class Polygon;
class Bitmap : public AutoDeletingReferenceable {
public:
	Bitmap(uint16 width, uint16 height, uint16 bytesPerPixel);
	void ImportData(const void *data, uint32 width, uint32 height);

	// Locking
	bool Lock() const;
	void Unlock() const;

	void* Pixels() const;
	uint32 GetPixel(uint16 x, uint16 y) const;

	// Attributes
	GFX::rect Frame() const;
	uint16 Width() const;
	uint16 Height() const;
	uint16 Pitch() const;
	uint16 BitsPerPixel() const;

	void SetPosition(uint16 x, uint16 y);

	void Clear(uint32 color = 0);

	// Colors and palette
	void SetColors(GFX::Color* colors, uint8 start, int num);
	void SetColors(const GFX::Color& color, uint8 start, int num);
	void GetPalette(GFX::Palette& palette) const;
	void SetPalette(const GFX::Palette& palette);

	bool GetColorKey(uint32& colorKey) const;
	void SetColorKey(uint32 key, bool on = true);
	void SetColorKey(uint8 r, uint8 g, uint8 b, bool on = true);
	void ClearColorKey() { SetColorKey(0, false); };
	void SetAlpha(uint8 alphaValue, bool on = true);

	// Drawing
	void PutPixel(int32 x, int32 y, const uint32 color);
	void StrokeLine(int32 xStart, int32 yStart,
			int32 xEnd, int32 yEnd, const uint32 color);
	void StrokeRect(const GFX::rect& rect, const uint32 color);
	void StrokePolygon(const Polygon& polygon, const uint32 color);
	void StrokePolygon(const Polygon& polygon, const uint32 color,
				int32 xOffset, int32 yOffset);

	void FillRect(const GFX::rect& rect, const uint32 color);
	void FillPolygon(const Polygon& polygon, const uint32 color);
	void FillPolygon(const Polygon& polygon, const uint32 color,
					int32 xOffset, int32 yOffset);

	void StrokeCircle(const int16& centerX, const int16& centerY,
					const uint32 radius, const uint32 color);
	void FillCircle(const int16& centerX, const int16& centerY,
					const uint32 radius, const uint32 color);

	// Blitting
	void BlitTo(Bitmap* target, const GFX::point& where) const;

	uint32 MapColor(const uint8 r, const uint8 g, const uint8 b);

	// Flip / Mirror
	Bitmap* GetMirrored() const;
	void Mirror();
	void Flip();

	Bitmap* Clone() const;

	// Load / Save
	static Bitmap* Load(const char* fileName);
	static Bitmap* Load(const void* data, const size_t size);

	void Save(const char* fileName) const;
	void Dump() const;

protected:
	friend class GraphicsEngine;

	SDL_Surface* fSurface;
	Bitmap* fMirrored;

	GFX::point fOffset;

	bool fOwnsSurface;

	Bitmap(SDL_Surface* surface, bool ownsSurface = true);
	virtual ~Bitmap();

	SDL_Surface* Surface() const;
};


class DataBitmap : public Bitmap {
public:
	DataBitmap(void* data, uint16 width, uint16 height, uint16 depth, bool ownsData);
	virtual ~DataBitmap();
private:
	uint8* fData;
	bool fOwns;
};

#endif /* BITMAP_H_ */
