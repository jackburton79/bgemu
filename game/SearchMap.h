/*
 * Copyright 2018, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _SEARCHMAP_H
#define _SEARCHMAP_H

#include "SupportDefs.h"

#include <map>

class Bitmap;
class SearchMap {
public:
	SearchMap(std::string name);
	~SearchMap();

	int32 Width() const;
	int32 Height() const;
	void SetRatios(int32 h, int32 v);

	bool IsPointPassable(int32 x, int32 y) const;

	void SetPoint(int32 x, int32 y);
	void ClearPoint(int32 x, int32 y);
	
	Bitmap* Image();
private:
	Bitmap* fImage;
	Bitmap* fModifiedMap;
	
	int32 fHRatio;
	int32 fVRatio;
};


#endif // _SEARCHMAP_H
