#ifndef __TILE_H
#define __TILE_H

#include "Types.h"

#include <vector>

#include <SDL.h>

class Door;
class MapOverlay;
class TileMap {
public:
	TileMap();

	void AddTileIndex(uint16 index);
	uint16 TileIndex();

	void SetSecondaryTileIndex(uint16 index);
	uint16 SecondaryTileIndex() const;

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


class Tile {
public:
	Tile(uint32 index);
	void Draw(SDL_Surface *surface, SDL_Rect *rect, bool full = false);

	void AddTileMap(TileMap *map);
	void SetDoor(Door *d);

	::Door *Door() const;

	void MouseOver();
	void Clicked();

private:
	::Door *fDoor;
	std::vector<class TileMap*> fTileMap;
};


#endif // __TILE_H
