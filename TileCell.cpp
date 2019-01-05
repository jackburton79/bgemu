#include "Bitmap.h"
#include "Door.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "ResManager.h"
#include "TileCell.h"
#include "Timer.h"
#include "TisResource.h"

#include "WedResource.h" // TODO: Remove once WedOverlay is moved

#include <assert.h>

static GFX::Color sTransparentColor = { 0, 255, 0 };

TileCell::TileCell(uint32 number, std::vector<MapOverlay*>& overlays, int numOverlays)
	:
	fNumber(number),
	fDoor(NULL),
	fOverlays(overlays),
	fNumOverlays(numOverlays)
{
}


TileCell::~TileCell()
{
	std::list<Object*>::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++)
		(*i)->SetTileCell(NULL);
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


static inline bool
ShouldDrawOverlay(const int i, const int mask)
{
	return i == 0 || (mask & (1 << i)) != 0;
}


void
TileCell::Draw(Bitmap* bitmap, GFX::rect *rect, bool advanceFrame, bool full)
{
	MapOverlay* overlayZero = fOverlays[0];
	if (overlayZero == NULL)
		return;

	TileMap* tileMapZero = overlayZero->TileMapForTileCell(fNumber);
	if (tileMapZero == NULL) {
		std::cerr << "Tilemap Zero is NULL!" << std::endl;
		return;
	}
	const int8 mask = tileMapZero->Mask();
	int maxOverlay = full ? fNumOverlays : 1;
	for (int i = maxOverlay - 1; i >= 0; i--) {
		if (!ShouldDrawOverlay(i, mask))
			continue;
	    MapOverlay *overlay = fOverlays[i];
		TileMap *map = overlay->TileMapForTileCell(i == 0 ? fNumber : 0);
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

		TISResource *tis = gResManager->GetTIS(overlay->TileSet());
		Bitmap *cell = tis->TileAt(index);
		assert(cell != NULL);

		gResManager->ReleaseResource(tis);

		GFX::Color *color = NULL;
		if (i == 0 && mask != 0) {
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
	MapOverlay* overlayZero = fOverlays[0];
	if (overlayZero == NULL)
		return;

	TileMap* tileMapZero = overlayZero->TileMapForTileCell(fNumber);
	if (tileMapZero == NULL)
		return;

	const int8 mask = tileMapZero->Mask();
	for (int i = fNumOverlays - 1; i >= 0; i--) {
		if (!ShouldDrawOverlay(i, mask))
			continue;
		MapOverlay *overlay = fOverlays[i];
		TileMap *map = overlay->TileMapForTileCell(i == 0 ? fNumber : 0);
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
	fDoor->SetTileCell(this);
}


void
TileCell::SetObject(Object* object)
{
	fObjects.push_back(object);
}


void
TileCell::RemoveObject(Object* object)
{
	std::list<Object*>::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		if (object == (*i)) {
			fObjects.remove(object);
			break;
		}
	}
}


uint32
TileCell::GetObjects(std::vector<Object*>& objects)
{
	std::list<Object*>::iterator i;
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


// TileMap
TileMap::TileMap()
	:
	fSecondaryIndex(-1),
	fCurrentIndex(0),
	fMask(0)
{
}


void
TileMap::AddTileIndex(int16 index)
{
	fIndices.push_back(index);
}


int16
TileMap::TileIndex(bool advanceFrame)
{
	int16 index = fIndices[fCurrentIndex];
	if (advanceFrame && ++fCurrentIndex >= fIndices.size())
		fCurrentIndex = 0;
	return index;
}


void
TileMap::SetSecondaryTileIndex(int16 index)
{
	fSecondaryIndex = index;
}


int16
TileMap::SecondaryTileIndex() const
{
	return fSecondaryIndex;
}


void
TileMap::SetMask(uint8 mask)
{
	fMask = mask;
}


uint8
TileMap::Mask() const
{
	return fMask;
}
