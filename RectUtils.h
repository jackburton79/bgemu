#ifndef __RECTUTILS_H
#define __RECTUTILS_H

#include "IETypes.h"

static inline GFX::rect
offset_rect(const GFX::rect &rect, sint16 x, sint16 y)
{
	GFX::rect newRect = rect;
	newRect.x += x;
	newRect.y += y;
	return newRect;
}


static inline IE::point
offset_point(const IE::point &point, sint16 x, sint16 y)
{
	IE::point newPoint = point;
	newPoint.x += x;
	newPoint.y += y;
	return newPoint;
}


static inline bool
rects_intersect(const GFX::rect &rectA, const GFX::rect &rectB)
{
	return !(rectA.x > rectB.x + rectB.w
			|| rectA.y > rectB.y + rectB.h
			|| rectA.x + rectA.w < rectB.x
			|| rectA.y + rectA.h < rectB.y);
}

#endif // __RECTUTILS_H
