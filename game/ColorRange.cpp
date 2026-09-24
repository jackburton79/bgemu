/*
 * ColorRange.cpp
 *
 *  Created on: 20 ago 2026
 */

#include "ColorRange.h"

#include <iostream>

#include "BmpResource.h"
#include "Bitmap.h"
#include "CreResource.h"
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



void
ApplyPaperdollColors(GFX::Palette& palette, const CREColors& colors)
{
	ApplyRange(palette, 0x04, colors.metal);
	ApplyRange(palette, 0x10, colors.minor);
	ApplyRange(palette, 0x1c, colors.major);
	ApplyRange(palette, 0x28, colors.skin);
	ApplyRange(palette, 0x34, colors.leather);
	ApplyRange(palette, 0x40, colors.armor);
	ApplyRange(palette, 0x4c, colors.hair);

	// 8 shades each, taken from the second entry of the range they repeat.
	struct Copy { uint8 to; uint8 from; };
	static const Copy kCopies[] = {
		{ 0x58, 0x11 }, { 0x60, 0x1d }, { 0x68, 0x11 }, { 0x70, 0x05 },
		{ 0x78, 0x35 }, { 0x80, 0x35 }, { 0x88, 0x11 },
		{ 0x90, 0x35 }, { 0x98, 0x35 }, { 0xa0, 0x35 },
		{ 0xb0, 0x29 },
		{ 0xb8, 0x35 }, { 0xc0, 0x35 }, { 0xc8, 0x35 }, { 0xd0, 0x35 },
		{ 0xd8, 0x35 }, { 0xe0, 0x35 }, { 0xe8, 0x35 }, { 0xf0, 0x35 },
		{ 0xf8, 0x35 },
	};
	for (const Copy& copy : kCopies) {
		for (int i = 0; i < 8; i++)
			palette.colors[copy.to + i] = palette.colors[copy.from + i];
	}
}
