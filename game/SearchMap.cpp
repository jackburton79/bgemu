/*
 * Copyright 2018, Stefano Ceccherini <stefano.ceccherini@gmail.com>
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
	// Search tile is 1/16 and 1/12
	x = x / 16;
	y = y / 12;
	
	uint8 state = fModifiedMap->GetPixel(x, y);
	switch (state) {
		case 0:
		case 8:
		case 10:
		case 12:
		case 13:
			return false;
		default:
			break;
	}
	
	return true;
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
