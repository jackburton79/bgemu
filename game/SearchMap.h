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
