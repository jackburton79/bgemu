/*
 * Copyright 2018, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _SEARCHMAP_H
#define _SEARCHMAP_H

#include "SupportDefs.h"


class Bitmap;
class SearchMap {
public:
	SearchMap(std::string name);
	~SearchMap();

	int32 Width() const;
	int32 Height() const;

	bool IsPointPassable(int32 x, int32 y) const;

	// Whether the terrain at this pixel blocks line of sight - the
	// area's own static WED search-map classification (0=Obstacle and
	// 10=Wall, see IESDP appendices/search.htm's color table; 8 is
	// explicitly documented as an impassable obstacle that does *not*
	// block light, e.g. a low fence/furniture, so it's deliberately
	// excluded). Always reads the original, never-modified bitmap
	// (fImage, not fModifiedMap) - a door's own open/closed blocking
	// (SetCellBlocked()/SetCellPassable(), see Door.cpp) only tracks
	// passability, not light-blocking, so doors aren't accounted for
	// here; a closed door won't block sight through its cell.
	bool BlocksLight(int32 x, int32 y) const;

	// Search-map value 14 ("Worldmap exit", appendices/search.htm) - a
	// wilderness/outdoor area's own map-edge cells authored to open the
	// worldmap when a party member walks there, entirely separate from
	// a Region (there's no polygon object for this at all - it's baked
	// directly into the search-map bitmap). Reads the original,
	// never-modified bitmap, same reasoning as BlocksLight().
	bool IsWorldmapExit(int32 x, int32 y) const;

	void SetPoint(int32 x, int32 y);
	void ClearPoint(int32 x, int32 y);
	void ForcePassable(int32 x, int32 y);

	// Cell-space variants (already-divided-by-cell-size coordinates) -
	// see SearchMap.cpp for why these exist separately from the pixel-
	// space methods above.
	void SetCellBlocked(int32 cellX, int32 cellY);
	void SetCellPassable(int32 cellX, int32 cellY);

	Bitmap* Image();
private:
	Bitmap* fImage;
	Bitmap* fModifiedMap;

	std::vector<uint8> fPassabilityMap;
	int32 fWidth;
	int32 fHeight;
};


#endif // _SEARCHMAP_H
