/*
 * ColorRange.h
 *
 *  Created on: 20 ago 2026
 */

#pragma once

#include "GraphicsDefs.h"

#include <vector>

struct ColorRange {
	GFX::Color shade[12];
};


bool InitColorRanges();
void ApplyRange(GFX::Palette& palette, uint8 start, uint8 rangeIndex);
