/*
 * BackMap.cpp
 *
 *  Created on: 06/gen/2017
 *      Author: Stefano Ceccherini
 */


#include "BackMap.h"
#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "IETypes.h"
#include "TileCell.h"
#include "WedResource.h"

#include <assert.h>
#include <vector>


BackMap::BackMap(WEDResource* wed)
	:
	fImage(NULL),
	fMapWidth(0),
	fMapHeight(0),
	fTileWidth(TILE_WIDTH),
	fTileHeight(TILE_HEIGHT)
{
	if (_LoadOverlays(wed)) {
		fMapWidth = fOverlays[0]->Width();
		fMapHeight = fOverlays[0]->Height();
	}

	std::cout << "Initializing Tile Cells...";
	std::flush(std::cout);
	uint32 numTiles = fOverlays[0]->Size();
	for (uint16 i = 0; i < numTiles; i++) {
		fTileCells.push_back(new TileCell(i, fOverlays, fOverlays.size()));
	}
	std::cout << "Done! Loaded " << numTiles << " tile cells!" << std::endl;

	fImage = new Bitmap(fMapWidth * fTileWidth,
			fMapHeight * fTileHeight, 16);
}


BackMap::~BackMap()
{
	if (fImage != NULL)
		fImage->Release();

	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();

	for (uint32 c = 0; c < fOverlays.size(); c++)
		delete fOverlays[c];
	fOverlays.clear();
}


uint16
BackMap::Width() const
{
	return fMapWidth;
}


uint16
BackMap::Height() const
{
	return fMapHeight;
}


TileCell*
BackMap::TileAt(uint16 x, uint16 y)
{
	uint16 numTile = y + x;
	if (numTile >= fTileCells.size())
		return NULL;

	return fTileCells[numTile];
}


TileCell*
BackMap::TileAt(uint16 index)
{
	return fTileCells[index];
}


int32
BackMap::CountTiles() const
{
	return fTileCells.size();
}


TileCell*
BackMap::TileAtPoint(const IE::point& point)
{
	uint16 num = TileNumberForPoint(point);
	return fTileCells[num];
}


uint16
BackMap::TileNumberForPoint(const IE::point& point)
{
	const uint16 overlayWidth = fOverlays[0]->Width();
	const uint16 tileX = point.x / TILE_WIDTH;
	const uint16 tileY = point.y / TILE_HEIGHT;

	return tileY * overlayWidth + tileX;
}


void
BackMap::Update(GFX::rect rect)
{
	if (fImage == NULL)
		return;
	assert(fOverlays[0]);

	fImage->Clear(0);

	const uint16 firstTileX = rect.x / fTileWidth;
	const uint16 firstTileY = rect.y / fTileHeight;
	uint16 lastTileX = firstTileX + (rect.w / fTileWidth) + 2;
	uint16 lastTileY = firstTileY + (rect.h / fTileHeight) + 2;

	lastTileX = std::min(lastTileX, fMapWidth);
	lastTileY = std::min(lastTileY, fMapHeight);

	bool advance = true;
	//bool advance = Timer::Get("ANIMATEDTILES")->Expired();
	GFX::rect tileRect(0, 0, fTileWidth, fTileHeight);
	for (uint16 y = 0; y < fMapHeight; y++) {
		tileRect.w = fTileWidth;
		tileRect.h = fTileHeight;
		tileRect.y = y * fTileHeight - rect.y;

		const uint32 tileNumY = y * fMapWidth;
		for (uint16 x = 0; x < fMapWidth; x++) {
			tileRect.w = fTileWidth;
			tileRect.x = x * fTileWidth - rect.x;

			TileCell* tile = TileAt(tileNumY,  x);
			if (tile == NULL) {
				continue;
			}
			if (advance)
				tile->AdvanceFrame();
			if (y >= firstTileY && y <= lastTileY
					&& x >= firstTileX && x <= lastTileX)
				tile->Draw(fImage, &tileRect, false, true);
		}
	}
}


Bitmap*
BackMap::Image() const
{
	return fImage;
}


bool
BackMap::_LoadOverlays(WEDResource* wed)
{
	std::cout << "Loading Overlays...";
	std::flush(std::cout);
	uint32 numOverlays = wed->CountOverlays();
	for (uint32 i = 0; i < numOverlays; i++) {
		MapOverlay *overlay = wed->GetOverlay(i);
		fOverlays.push_back(overlay);
	}
	std::cout << "Done! Loaded " << numOverlays << " overlays. ";
	std::cout << "Map size: " << fOverlays[0]->Width();
	std::cout << "x" << fOverlays[0]->Height() << std::endl;

	return true;
}
