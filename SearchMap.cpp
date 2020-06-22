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
	fModifiedMap(NULL),
	fHRatio(1),
	fVRatio(1)
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


void
SearchMap::SetRatios(int32 h, int32 v)
{
	fHRatio = h;
	fVRatio = v;
}


bool
SearchMap::IsPointPassable(int32 x, int32 y) const
{
	x = ceilf(x / fHRatio);
	y = ceilf(y / fVRatio);
	
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
	return;
	x = ceilf(x / fHRatio);
	y = ceilf(y / fVRatio);
	fModifiedMap->PutPixel(x, y, 0);
}


void
SearchMap::ClearPoint(int32 x, int32 y)
{
	return;
	x = ceilf(x / fHRatio);
	y = ceilf(y / fVRatio);
	fModifiedMap->PutPixel(x, y, fImage->GetPixel(x, y));
}


Bitmap*
SearchMap::Image()
{
	return fModifiedMap;
}
