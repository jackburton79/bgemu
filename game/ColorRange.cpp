/*
 * ColorRange.cpp
 *
 *  Created on: 20 ago 2026
 */

#include "ColorRange.h"

#include <iostream>

#include "BmpResource.h"
#include "Bitmap.h"
#include "ResManager.h"

static std::vector<ColorRange> sColorRanges;

// Use MPALETTE.BMP (12 wide, one row per CRE color-index byte) for
// per-creature recoloring gradient table
bool
InitColorRanges()
{
	BMPResource* ranges = gResManager->GetBMP("MPALETTE");
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

	return true;
}


// A CRE's color byte is untrusted data (0-255) with no guarantee it
// falls inside MPALETTE.BMP's own row count
void
ApplyRange(GFX::Palette& palette, uint8 start, uint8 rangeIndex)
{
	if (sColorRanges.empty())
		return;
	if (rangeIndex >= sColorRanges.size())
		rangeIndex = 0;

	const ColorRange& range = sColorRanges[rangeIndex];

	for (int i = 0; i < 12; i++)
		palette.colors[start + i] = range.shade[i];
}

