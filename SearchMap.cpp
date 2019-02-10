/*
 * Copyright 2018, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SearchMap.h"

#include "Bitmap.h"
#include "BmpResource.h"
#include "ResManager.h"

SearchMap::SearchMap(std::string name)
	:
	fImage(NULL)
{
	BMPResource* resource = gResManager->GetBMP(name.c_str());
	if (resource != NULL) {
		fImage = resource->Image();
		gResManager->ReleaseResource(resource);
	}
}


SearchMap::~SearchMap()
{
	fImage->Release();
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
	x = x / fHRatio;
	y = y / fVRatio;
	
	uint8 state = fImage->GetPixel(x, y);
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

	try {
		return fBusyPoints.at(std::make_pair(x, y));
	} catch (...) {
	}
	return true;
}


void
SearchMap::SetPoint(int32 x, int32 y)
{
	x = x / fHRatio;
	y = y / fVRatio;
	fBusyPoints[std::make_pair(x, y)] = true;
}


void
SearchMap::ClearPoint(int32 x, int32 y)
{
	x = x / fHRatio;
	y = y / fVRatio;
	fBusyPoints[std::make_pair(x, y)] = false;
}
