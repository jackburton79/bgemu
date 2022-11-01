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
#include "Log.h"
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
	_LoadOverlays(wed);

	std::cout << "Initializing Tile Cells...";
	std::flush(std::cout);
	uint32 numCells = fOverlays[0]->Size();
	for (uint16 i = 0; i < numCells; i++) {
		fTileCells.push_back(new TileCell(i, fOverlays));
	}
	std::cout << "Done! Loaded " << numCells << " tile cells!" << std::endl;

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
BackMap::TileAt(uint16 index)
{
	if (index >= fTileCells.size())
		return NULL;
	return fTileCells[index];
}


int32
BackMap::CountTiles() const
{
	return fTileCells.size();
}


// Get the tile at the specified (GLOBAL) coordinate
TileCell*
BackMap::TileAtPoint(const IE::point& point)
{
	const int32 num = TileNumberForPoint(point);
	try {
		return fTileCells.at(num);
	} catch (std::exception& e) {
		return NULL;
	}
}


int32
BackMap::TileNumberForPoint(const IE::point& point) const
{
	const uint16 tileX = point.x / fTileWidth;
	const uint16 tileY = point.y / fTileHeight;

	return tileY * fMapWidth + tileX;
}


void
BackMap::Update(GFX::rect rect, bool allOverlays)
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
		tileRect.y = y * fTileHeight - rect.y;
		const uint32 tileNumY = y * fMapWidth;
		for (uint16 x = 0; x < fMapWidth; x++) {
			tileRect.x = x * fTileWidth - rect.x;
			const uint16 tileIndex = tileNumY + x;
			TileCell* tile = TileAt(tileIndex);
			if (tile == NULL) {
				std::cerr << Log::Red << "Tile " << tileIndex << " not found!" << std::endl;
				continue;
			}
			if (advance)
				tile->AdvanceFrame();
			if (y >= firstTileY && y <= lastTileY
					&& x >= firstTileX && x <= lastTileX)
				tile->Draw(fImage, tileRect, false, allOverlays);
		}
	}
}


Bitmap*
BackMap::Image() const
{
	return fImage;
}


void
BackMap::_LoadOverlays(WEDResource* wed)
{
	std::cout << "Loading Overlays...";
	std::flush(std::cout);
	uint32 numOverlays = wed->CountOverlays();
	for (uint32 i = 0; i < numOverlays; i++) {
		MapOverlay *overlay = wed->GetOverlay(i);
		fOverlays.push_back(overlay);
	}

	fMapWidth = fOverlays[0]->Width();
	fMapHeight = fOverlays[0]->Height();

	std::cout << "Done! Loaded " << numOverlays << " overlays. ";
	std::cout << "Map size: " << fMapWidth << "x" << fMapHeight;
	std::cout << " (" << fMapWidth * fTileWidth << "x" << fMapHeight * fTileHeight << ")" << std::endl;
	for (uint32 i = 1; i < numOverlays; i++) {
		std::cout << "overlay " << i << ": ";
		std::cout << fOverlays[i]->Width() << "x" << fOverlays[i]->Height() << std::endl;
	}
}
