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

BackMap::BackMap(std::vector<MapOverlay*>& overlays,
		uint16 mapWidth, uint16 mapHeight,
		uint16 tileWidth, uint16 tileHeight)
	:
	fImage(NULL),
	fMapWidth(mapWidth),
	fMapHeight(mapHeight),
	fTileWidth(tileWidth),
	fTileHeight(tileHeight)
{
	fImage = new Bitmap(mapWidth * tileWidth,
			mapHeight * tileHeight, 16);

	std::cout << "Initializing Tile Cells...";
	std::flush(std::cout);
	uint32 numTiles = overlays[0]->Size();
	for (uint16 i = 0; i < numTiles; i++) {
		fTileCells.push_back(new TileCell(i, overlays, overlays.size()));
	}
	std::cout << "Done! Loaded " << numTiles << " tile cells!" << std::endl;
}


BackMap::~BackMap()
{
	if (fImage != NULL)
		fImage->Release();

	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();
}


TileCell*
BackMap::TileAt(uint16 x, uint16 y)
{
	return fTileCells[y + x];
}


void
BackMap::Update(MapOverlay* overlay, GFX::rect rect)
{
	assert(overlay);

	fImage->Clear(0);

	const uint16 overlayWidth = overlay->Width();
	const uint16 firstTileX = rect.x / fTileWidth;
	const uint16 firstTileY = rect.y / fTileHeight;
	uint16 lastTileX = firstTileX + (rect.w / fTileWidth) + 2;
	uint16 lastTileY = firstTileY + (rect.h / fTileHeight) + 2;

	lastTileX = std::min(lastTileX, overlayWidth);
	lastTileY = std::min(lastTileY, overlay->Height());

	bool advance = true;
	//bool advance = Timer::Get("ANIMATEDTILES")->Expired();
	GFX::rect tileRect(0, 0, fTileWidth, fTileHeight);
	for (uint16 y = 0; y < overlay->Height(); y++) {
		tileRect.w = fTileWidth;
		tileRect.h = fTileHeight;
		tileRect.y = y * fTileHeight - rect.y;

		const uint32 tileNumY = y * overlayWidth;
		for (uint16 x = 0; x < overlayWidth; x++) {
			tileRect.w = fTileWidth;
			tileRect.x = x * fTileWidth - rect.x;

			TileCell* tile = TileAt(tileNumY,  x);
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

