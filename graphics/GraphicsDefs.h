/*
 * GraphicsDefs.h
 *
 *  Created on: 03/apr/2015
 *      Author: stefano
 */

#ifndef GRAPHICSDEFS_H_
#define GRAPHICSDEFS_H_

#include "SupportDefs.h"

namespace GFX {

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};


struct Palette {
	Color colors[256];
};

struct point {
	int16 x;
	int16 y;
};

struct rect {
	sint16 x;
	sint16 y;
	uint16 w;
	uint16 h;

	rect();
	rect(sint16 x, sint16 y, uint16 width, uint16 height);
	void Print() const;
};

};

#endif /* GRAPHICSDEFS_H_ */
