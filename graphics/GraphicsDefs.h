/*
 * GraphicsDefs.h
 *
 *  Created on: 03/apr/2015
 *      Author: stefano
 */

#ifndef GRAPHICSDEFS_H_
#define GRAPHICSDEFS_H_

#include "SupportDefs.h"

struct SDL_Rect;

namespace GFX {

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};


class Palette {
public:
	Palette();
	Palette(const GFX::Color& start, const GFX::Color& end);
	void ModColor(uint8 index, uint8 mod);
	void Dump() const;
	void Dump(const char* fileName) const;
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

void GFXRectToSDLRect(const rect* source, SDL_Rect* dest);
void SDLRectToGFXRect(const SDL_Rect* source, rect* dest);

};

#endif /* GRAPHICSDEFS_H_ */
