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
	x = x / 16;
	y = y / 12;
	if (x < 0 || x >= fWidth || y < 0 || y >= fHeight)
		return;

	fPassabilityMap[y * fWidth + x] = false;
	fModifiedMap->PutPixel(x, y, 0);
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


Bitmap*
SearchMap::Image()
{
	return fModifiedMap;
}
