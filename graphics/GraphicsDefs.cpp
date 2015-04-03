/*
 * GraphicsDefs.cpp
 *
 *  Created on: 03/apr/2015
 *      Author: stefano
 */

#include "GraphicsDefs.h"

#include <iostream>

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

}


