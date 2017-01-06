/*
 * BackMap.h
 *
 *  Created on: 06/gen/2017
 *      Author: Stefano Ceccherini
 */

#ifndef BACKMAP_H_
#define BACKMAP_H_

#include <vector>

#include "GraphicsDefs.h"
#include "IETypes.h"

class Bitmap;
class MapOverlay;
class TileCell;
class BackMap {
public:
	BackMap(std::vector<MapOverlay*>& overlays,
			uint16 mapWidth, uint16 mapHeight,
			uint16 tileWidth, uint16 tileHeight);
	~BackMap();

	TileCell* TileAt(uint16 x, uint16 y);
	void Update(MapOverlay* overlay, GFX::rect rect);
	Bitmap* Image() const;

private:
	Bitmap* fImage;
	std::vector<TileCell*> fTileCells;
	uint16 fMapWidth;
	uint16 fMapHeight;
	uint16 fTileWidth;
	uint16 fTileHeight;
};



#endif /* BACKMAP_H_ */
