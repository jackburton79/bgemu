/*
 * Copyright 2018-2026, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchMap.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "ResManager.h"

#include <math.h>


// Search-map pixel palette indices that mean "not passable" - see
// SearchMap's constructor (original per-pixel classification) and
// ClearPoint() (restoring a pixel to its original classification).
static bool
_IsPixelPassable(uint8 pixel)
{
	return pixel != 0 && pixel != 8 && pixel != 10 && pixel != 12 && pixel != 13;
}


SearchMap::SearchMap(std::string name)
	:
	fImage(NULL),
	fModifiedMap(NULL)
{
	BMPResource* resource = gResManager->GetBMP(name.c_str());
	if (resource != NULL) {
		fImage = resource->Image();
		fModifiedMap = fImage->Clone();
		gResManager->ReleaseResource(resource);
	}

	if (fImage != nullptr) {
		fWidth = fImage->Width();
		fHeight = fImage->Height();

		fPassabilityMap.resize(fWidth * fHeight);

		for (int y = 0; y < fHeight; ++y) {
			for (int x = 0; x < fWidth; ++x) {
				uint8 state = fModifiedMap->GetPixel(x, y);
				fPassabilityMap[y * fWidth + x] = _IsPixelPassable(state);
			}
		}
	}
}


SearchMap::~SearchMap()
{
	fImage->Release();
	fModifiedMap->Release();
}


int32
SearchMap::Width() const
{
	return fImage->Width();
}


int32
SearchMap::Height() const
{
	return fImage->Height();
}


bool
SearchMap::IsPointPassable(int32 x, int32 y) const
{
	x /= 16;
	y /= 12;
	if (x < 0 || x >= fWidth || y < 0 || y >= fHeight)
		return false;
	return fPassabilityMap[y * fWidth + x];
}


void
SearchMap::SetPoint(int32 x, int32 y)
{
	SetCellBlocked(x / 16, y / 12);
}


void
SearchMap::ClearPoint(int32 x, int32 y)
{
	x = x / 16;
	y = y / 12;
	if (x < 0 || x >= fWidth || y < 0 || y >= fHeight)
		return;

	uint8 originalPixel = fImage->GetPixel(x, y);
	fPassabilityMap[y * fWidth + x] = _IsPixelPassable(originalPixel);
	fModifiedMap->PutPixel(x, y, originalPixel);
}


// Unlike ClearPoint() (restores whatever the area's own WED search-map
// bitmap originally had there - correct for actor occupancy, which
// should fall back to the real underlying terrain once vacated), this
// unconditionally marks the cell walkable regardless of what's under
// it. Needed for an open door's own footprint: per IESDP's search-map
// color table (appendices/search.htm), a doorway's base terrain is
// authored as "0 - Obstacle" or "10 - Wall" (both impassable) since the
// door object is meant to override it dynamically - ClearPoint() just
// puts that same wall/obstacle value straight back, so the "open"
// side never actually became walkable (found verifying Door::Open()/
// Close() with Check-Passable: passability was identical before and
// after opening a real door).
void
SearchMap::ForcePassable(int32 x, int32 y)
{
	SetCellPassable(x / 16, y / 12);
}


// Cell-space variants of SetPoint()/ForcePassable() above - for callers
// that already have search-map-native cell coordinates (e.g. a door's
// "impeded cell block" data, see IESDP are_v1.htm: "these entries are
// x.y coordinates in the area search map", i.e. already in cell units,
// not area pixels) and shouldn't have them divided by the cell size a
// second time.
void
SearchMap::SetCellBlocked(int32 cellX, int32 cellY)
{
	if (cellX < 0 || cellX >= fWidth || cellY < 0 || cellY >= fHeight)
		return;

	fPassabilityMap[cellY * fWidth + cellX] = false;
	fModifiedMap->PutPixel(cellX, cellY, 0);
}


void
SearchMap::SetCellPassable(int32 cellX, int32 cellY)
{
	if (cellX < 0 || cellX >= fWidth || cellY < 0 || cellY >= fHeight)
		return;

	fPassabilityMap[cellY * fWidth + cellX] = true;
	// 1 - Sand: an arbitrary but always-passable value (see
	// appendices/search.htm), just for fModifiedMap's own visual/debug
	// consistency - fPassabilityMap above is what pathfinding actually
	// reads.
	fModifiedMap->PutPixel(cellX, cellY, 1);
}


Bitmap*
SearchMap::Image()
{
	return fModifiedMap;
}
