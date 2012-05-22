#include "Door.h"
#include "ResManager.h"
#include "TileCell.h"
#include "TisResource.h"

#include "WedResource.h" // TODO: Remove once WedOverlay is moved


TileCell::TileCell(uint32 number)
	:
	fDoor(NULL)
{
	fTileMap.reserve(8);
}


void
_DrawOverlay(SDL_Surface *surface, SDL_Surface *cell,
		SDL_Rect rect, SDL_Color *color)
{
	if (color) {
		SDL_SetColorKey(cell, SDL_SRCCOLORKEY,
				SDL_MapRGB(cell->format, color->r, color->g, color->b));
	}

	SDL_BlitSurface(cell, NULL, surface, &rect);
}


void
TileCell::Draw(SDL_Surface *surface, SDL_Rect *rect, bool full)
{
	int maxOverlay = full ? fTileMap.size() : 1;

	SDL_Color trans = { 0, 255, 0 };
	for (int i = maxOverlay - 1; i >= 0; i--) {
	    // Check if this overlay needs to be drawn
	    if (i != 0 && (fTileMap[0]->Mask() & (1 << i)) == 0)
	    	continue;
	    bool closed = false;
		::TileMap *map = fTileMap[i];
		uint16 index;
		if (fDoor != NULL && !fDoor->Opened()) {
			closed = true;
			index = map->SecondaryTileIndex();
		} else
			index = map->TileIndex();

		MapOverlay *overlay = map->Overlay();
		if (overlay == NULL)
			throw "NULL Overlay";
		TISResource *tis = gResManager->GetTIS(overlay->TileSet());
		SDL_Surface *cell = tis->TileCellAt(index);
		if (cell == NULL) {
			// TODO: Fix this. Shouldn't request an invalid cell
			cell = SDL_CreateRGBSurface(SDL_SWSURFACE, 64, 64, 8, 0, 0, 0, 0);
			SDL_FillRect(cell, NULL, 3000);
		}
		SDL_Color *color = NULL;
		/*if (i == 0 && fTileMap[0]->Mask() != 0) {
			color = &trans;
			//color = &cell->format->palette->colors[255];
		}*/
		/*if (closed)
			SDL_FillRect(cell, NULL, 3000);*/
		_DrawOverlay(surface, cell, *rect, color);

		SDL_FreeSurface(cell);

		gResManager->ReleaseResource(tis);
	}
}


void
TileCell::SetTileMap(TileMap *map, int overlayNum)
{
	fTileMap[overlayNum] = map;
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


/*
TileMap *
Tile::TileMap() const
{
	return fTileMap;
}
*/

// TileMap
TileMap::TileMap()
	:
	fSecondaryIndex(0)
{
}


void
TileMap::AddTileIndex(uint16 index)
{
	fIndices.push_back(index);
}


uint16
TileMap::TileIndex()
{
	uint16 index = fIndices[fCurrentIndex];
	if (++fCurrentIndex >= fIndices.size())
		fCurrentIndex = 0;
	return index;
}


void
TileMap::SetSecondaryTileIndex(uint16 index)
{
	fSecondaryIndex = index;
}


uint16
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


void
TileMap::SetOverlay(MapOverlay *overlay)
{
	fOverlay = overlay;
}


MapOverlay *
TileMap::Overlay() const
{
	return fOverlay;
}
