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
class WEDResource;
class BackMap {
public:
	BackMap(WEDResource* wed);
	~BackMap();

	uint16 Width() const;
	uint16 Height() const;

	TileCell* TileAt(uint16 x, uint16 y);
	TileCell* TileAt(uint16 index);
	TileCell* TileAtPoint(const IE::point& point);
	uint16 TileNumberForPoint(const IE::point& point);
	int32 CountTiles() const;


	void Update(GFX::rect rect, bool allOverlays);
	Bitmap* Image() const;

private:
	bool _LoadOverlays(WEDResource* wed);

	Bitmap* fImage;
	std::vector<MapOverlay*> fOverlays;
	std::vector<TileCell*> fTileCells;
	uint16 fMapWidth;
	uint16 fMapHeight;
	uint16 fTileWidth;
	uint16 fTileHeight;
};



#endif /* BACKMAP_H_ */
