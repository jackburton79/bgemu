#include "Actor.h"
#include "Bitmap.h"
#include "Door.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "Region.h"
#include "ResManager.h"
#include "TileCell.h"
#include "Timer.h"
#include "TisResource.h"

#include "WedResource.h" // TODO: Remove once WedOverlay is moved

#include <algorithm>
#include <assert.h>

static GFX::Color sTransparentColor = { 0, 255, 0 };

TileCell::TileCell(uint32 number, std::vector<MapOverlay*>& overlays, int numOverlays)
	:
	fNumber(number),
	fDoor(NULL),
	fOverlays(overlays),
	fPosX(-1),
	fPosY(-1),
	fOverlayMask(0)
{
	TileMap* map = overlays[0]->TileMapForTileCell(number);
	if (map != NULL)
		fOverlayMask = map->Mask();
}


TileCell::~TileCell()
{
	std::list<Actor*>::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++)
		(*i)->SetTileCell(NULL);
}


uint16
TileCell::ID() const
{
	return fNumber;
}


void
TileCell::SetPosition(uint16 x, uint16 y)
{
	fPosX = x;
	fPosY = y;
}


void
_DrawOverlay(Bitmap* dest, Bitmap *cell, GFX::rect rect, GFX::Color *color)
{
	if (color)
		cell->SetColorKey(color->r, color->g, color->b);

	GraphicsEngine::Get()->BlitBitmap(cell, NULL, dest, &rect);
}


void
TileCell::Draw(Bitmap* bitmap, GFX::rect *rect, bool advanceFrame, bool full)
{
	int maxOverlay = full ? fOverlays.size() : 1;
	for (int i = maxOverlay - 1; i >= 0; i--) {
		if (!_ShouldDrawOverlay(i))
			continue;
		if (fOverlays[i]->Size() == 0)
			continue;
		TileMap *map = fOverlays[i]->TileMapForTileCell(fNumber);
		if (map == NULL)
			continue;

		int16 index = map->TileIndex(advanceFrame);
		if (fDoor != NULL && !fDoor->Opened()) {
			int16 secondaryIndex = map->SecondaryTileIndex();
			if (secondaryIndex != -1)
				index = secondaryIndex;
			else {
				//std::cerr << "TileCell::Draw(): secondary index is -1. BUG?." << std::endl;
			}
		}

		TISResource *tis = gResManager->GetTIS(fOverlays[i]->TileSet());
		Bitmap *cell = tis->TileAt(index);
		assert(cell != NULL);

		gResManager->ReleaseResource(tis);

		GFX::Color *color = NULL;
		if (i == 0 && fOverlayMask != 0) {
			color = &sTransparentColor;
			//color = &cell->format->palette->colors[255];
		}

		_DrawOverlay(bitmap, cell, *rect, color);

		cell->Release();
	}
}


void
TileCell::AdvanceFrame()
{
	for (int i = fOverlays.size() - 1; i >= 0; i--) {
		if (!_ShouldDrawOverlay(i))
			continue;
		if (fOverlays[i]->Size() == 0)
			continue;
		TileMap *map = fOverlays[i]->TileMapForTileCell(fNumber);
		if (map != NULL)
			map->TileIndex(true);
	}
}


Door *
TileCell::Door() const
{
	return fDoor;
}


void
TileCell::SetDoor(::Door *d)
{
	fDoor = d;
	//fDoor->SetTileCell(this);
}


void
TileCell::ActorEnteredCell(Actor* object)
{
	object->Acquire();
	fObjects.push_back(object);
	for (std::vector<Region*>::iterator i = fRegions.begin();
		i != fRegions.end(); i++) {
		(*i)->ActorEntered(object);
	}
	// TODO: Check if actor entered regions which covers
	// this cell (in part or completely)
}


void
TileCell::ActorExitedCell(Actor* object)
{
	std::list<Actor*>::iterator actorIterator;
	for (actorIterator = fObjects.begin(); actorIterator != fObjects.end(); actorIterator++) {
		if (object == (*actorIterator)) {
			// Remove object from regions
			// TODO: Wrong. If region spans over multiple tiles,
			// the actor could still be inside region
			for (std::vector<Region*>::iterator regionIterator = fRegions.begin();
					regionIterator != fRegions.end(); regionIterator++) {
				(*regionIterator)->ActorExited(object);
			}
			fObjects.remove(object);
			break;
		}
	}
	object->Release();
}


uint32
TileCell::GetObjects(std::vector<Actor*>& objects)
{
	std::list<Actor*>::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++)
		objects.push_back(*i);
	return objects.size();
}


void
TileCell::Clicked()
{
	if (fDoor != NULL)
		fDoor->Toggle();
}


void
TileCell::MouseOver()
{
}


struct FindRegion {
	FindRegion(const Region* region)
		: toFind(region) {};

	bool operator() (const Region* region) {
		return strcmp(region->Name(), toFind->Name()) == 0;
	};
	const Region* toFind;
};


void
TileCell::AddRegion(Region* region)
{
	if (!HasRegion(region))
		fRegions.push_back(region);
}


void
TileCell::RemoveRegion(Region* region)
{
	std::vector<Region*>::iterator i =
		std::find_if(fRegions.begin(), fRegions.end(), FindRegion(region));
	if (i != fRegions.end())
		fRegions.erase(i);
}
	

bool
TileCell::HasRegion(Region* region) const
{
	std::vector<Region*>::const_iterator i =
		std::find_if(fRegions.begin(), fRegions.end(), FindRegion(region));
	return i != fRegions.end();
}


bool
TileCell::_ShouldDrawOverlay(const int overlayIndex) const
{
	return overlayIndex == 0 || (fOverlayMask & (1 << overlayIndex)) != 0;
}


// TileMap
TileMap::TileMap(uint8 mask, const std::vector<int16>& primaryIndexes, int16 secondaryIndex)
	:
	fSecondaryIndex(secondaryIndex),
	fCurrentIndex(0),
	fMask(mask)
{
	fIndices = primaryIndexes;
}


int16
TileMap::TileIndex(bool advanceFrame)
{
	int16 index = fIndices[fCurrentIndex];
	if (advanceFrame && ++fCurrentIndex >= fIndices.size())
		fCurrentIndex = 0;
	return index;
}


int16
TileMap::SecondaryTileIndex() const
{
	return fSecondaryIndex;
}


uint8
TileMap::Mask() const
{
	return fMask;
}
