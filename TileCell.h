#ifndef __TILE_H
#define __TILE_H

#include "IETypes.h"

#include <list>
#include <vector>

class Door;
class MapOverlay;
class Object;
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

	void SetPosition(uint16 x, uint16 y);
	void Draw(Bitmap* bitmap, GFX::rect *rect, bool advanceFrame, bool full = false);
	void AdvanceFrame();
	void SetDoor(::Door *d);
	::Door *Door() const;

	void SetObject(Object*);
	void RemoveObject(Object*);
	uint32 GetObjects(std::vector<Object*>& objects);

	void MouseOver();
	void Clicked();

private:
	uint32 fNumber;
	::Door *fDoor;
	std::vector<MapOverlay*>& fOverlays;

	// TODO: This should store the object IDs, since
	// storing the pointers is not safe
	std::list<Object*> fObjects;
	int fNumOverlays;
	uint16 fPosX;
	uint16 fPosY;
};


#endif // __TILE_H
