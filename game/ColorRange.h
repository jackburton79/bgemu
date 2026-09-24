/*
 * ColorRange.h
 *
 *  Created on: 20 ago 2026
 */

#pragma once

#include "GraphicsDefs.h"

#include <vector>

struct CREColors;

struct ColorRange {
	GFX::Color shade[12];
};


bool InitColorRanges();
void ApplyRange(GFX::Palette& palette, uint8 start, uint8 rangeIndex);


// Recolors a paperdoll BAM's palette from a creature's color bytes: the seven
// 12-entry gradient ranges (metal, minor, major, skin, leather, armor, hair),
// then the shade banks above them that repeat those ranges.
void ApplyPaperdollColors(GFX::Palette& palette, const CREColors& colors);
