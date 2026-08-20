/*
 * ColorRange.cpp
 *
 *  Created on: 20 ago 2026
 */

#include "ColorRange.h"

#include <assert.h>
#include <iostream>

#include "BmpResource.h"
#include "Bitmap.h"
#include "ResManager.h"

static std::vector<ColorRange> sColorRanges;

bool
InitColorRanges()
{
	std::cout << "InitColorRanges()" << std::endl;
	BMPResource* ranges = gResManager->GetBMP("RANGES12");
	if (ranges == nullptr)
		return false;
	Bitmap* bitmap = ranges->Image();
	ColorRange range;
	for (uint16 y = 0; y < bitmap->Height(); y++) {
		for (int x = 0; x < 12; x++) {
			uint32 pixel = bitmap->GetPixel(x, y);
			bitmap->GetRGBColor(pixel, range.shade[x].r, range.shade[x].g, range.shade[x].b);
		}
		sColorRanges.push_back(range);
	}
	bitmap->Release();

	gResManager->ReleaseResource(ranges);

	std::cout << "InitColorRanges(): OK" << std::endl;
	return true;
}


void
ApplyRange(GFX::Palette& palette, uint8 start, uint8 rangeIndex)
{
	assert(rangeIndex < 147);
	const ColorRange& range = sColorRanges[rangeIndex];

	for (int i = 0; i < 12; i++)
		palette.colors[start + i] = range.shade[i];
}

