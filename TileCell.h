#ifndef __TILE_H
#define __TILE_H

#include "IETypes.h"

#include <list>
#include <vector>

class Actor;
class Door;
class MapOverlay;
class Region;

class TileMap {
public:
	TileMap();

	void AddTileIndex(int16 index);
	int16 TileIndex(bool advanceFrame);

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
	TileCell(uint32 index, std::vector<MapOverlay*>& overlays, int numOverlays);
	~TileCell();

	uint16 ID() const;
	void SetPosition(uint16 x, uint16 y);
	void Draw(Bitmap* bitmap, GFX::rect *rect, bool advanceFrame, bool full = false);
	void AdvanceFrame();
	void SetDoor(::Door *d);
	::Door *Door() const;

	void ActorEnteredCell(Actor*);
	void ActorExitedCell(Actor*);
	uint32 GetObjects(std::vector<Actor*>& objects);

	void MouseOver();
	void Clicked();

	void AddRegion(Region* Region);
	void RemoveRegion(Region* region);
	bool HasRegion(Region* region) const;

	static uint32 GetTileCellsForRegion(std::vector<TileCell*>& cells,
										Region* region);

private:
	uint32 fNumber;
	::Door *fDoor;
	std::vector<MapOverlay*>& fOverlays;
	int fNumOverlays;
	uint16 fPosX;
	uint16 fPosY;

	// TODO: This should store the object IDs, since
	// storing the pointers is not safe
	
	std::list<Actor*> fObjects;
	std::vector<Region*> fRegions;
};


#endif // __TILE_H
