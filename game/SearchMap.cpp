/*
 * Copyright 2018-2026, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchMap.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "ResManager.h"

#include <math.h>

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
				bool passable = state != 0 && state != 8 && state != 10
						&& state != 12 && state != 13;

				fPassabilityMap[y * fWidth + x] = passable;
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
	return fPassabilityMap[y * fWidth + x];
}


void
SearchMap::SetPoint(int32 x, int32 y)
{
#if 1
	return;
#else
	x = x / 16;
	y = y / 12;
	fModifiedMap->PutPixel(x, y, 0);
#endif
}


void
SearchMap::ClearPoint(int32 x, int32 y)
{
#if 1
	return;
#else
	x = x / 16;
	y = y / 12;
	fModifiedMap->PutPixel(x, y, fImage->GetPixel(x, y));
#endif
}


Bitmap*
SearchMap::Image()
{
	return fModifiedMap;
}
