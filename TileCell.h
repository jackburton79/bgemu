#ifndef __TILE_H
#define __TILE_H

#include "IETypes.h"

#include <vector>

#include <SDL.h>

class Door;
class MapOverlay;
class TileMap {
public:
	TileMap();

	void AddTileIndex(int16 index);
	int16 TileIndex();

	void SetSecondaryTileIndex(int16 index);
	int16 SecondaryTileIndex() const;

	uint8 Mask() const;
	void SetMask(uint8 mask);

private:
	std::vector<int16> fIndices;
	int16 fSecondaryIndex;
	uint16 fCurrentIndex;
	uint8 fMask;
};


class TileCell {
public:
	TileCell(uint32 index, MapOverlay** overlays, int numOverlays);
	void Draw(SDL_Surface *surface, SDL_Rect *rect, bool full = false);

	void SetDoor(Door *d);
	::Door *Door() const;

	void MouseOver();
	void Clicked();

private:
	uint32 fNumber;
	::Door *fDoor;
	MapOverlay** fOverlays;
	int fNumOverlays;
};


#endif // __TILE_H
