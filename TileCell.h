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

	void SetOverlay(MapOverlay *overlay);
	MapOverlay *Overlay() const;

private:
	std::vector<int16> fIndices;
	int16 fSecondaryIndex;
	uint16 fCurrentIndex;
	MapOverlay *fOverlay;
	uint8 fMask;
};


class TileCell {
public:
	TileCell(uint32 index, );
	void Draw(SDL_Surface *surface, SDL_Rect *rect, bool full = false);

	void SetTileMap(TileMap *map, int overlayNum);
	void SetDoor(Door *d);

	::Door *Door() const;

	void MouseOver();
	void Clicked();

private:
	::Door *fDoor;
	std::vector<class TileMap*> fTileMap;
};


#endif // __TILE_H
