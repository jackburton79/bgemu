/*
 * GraphicsDefs.cpp
 *
 *  Created on: 03/apr/2015
 *      Author: stefano
 */

#include "GraphicsDefs.h"

#include <iostream>
#include <SDL.h>

namespace GFX {


rect::rect()
{
	x = y = w = h = 0;
}


rect::rect(sint16 _x, sint16 _y, uint16 width, uint16 height)
	:
	x(_x),
	y(_y),
	w(width),
	h(height)
{

}


void
rect::Print() const
{
	std::cout << std::dec;
	std::cout << "Rect: x: " << x << ", y: " << y;
	std::cout << ", w: " << w << ", h: " << h << std::endl;
}


void
GFXRectToSDLRect(const rect* source, SDL_Rect* dest)
{
	dest->x = (int)source->x;
	dest->y = (int)source->y;
	dest->w = (int)source->w;
	dest->h = (int)source->h;
}

void
SDLRectToGFXRect(const SDL_Rect* source, rect* dest)
{
	dest->x = (sint16)source->x;
	dest->y = (sint16)source->y;
	dest->w = (uint16)source->w;
	dest->h = (uint16)source->h;
}

}


