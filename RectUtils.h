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


static inline GFX::rect
offset_rect_to(const GFX::rect &rect, sint16 x, sint16 y)
{
	GFX::rect newRect = rect;
	newRect.x = x;
	newRect.y = y;
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


static inline bool
rect_contains(const GFX::rect& rect, const IE::point& point)
{
	if (point.x >= rect.x && point.x <= rect.x + rect.w
		&& point.y >= rect.y && point.y <= rect.y + rect.h)
		return true;
	return false;
}


static inline bool
rect_contains(const IE::rect& rect, const IE::point& point)
{
	if (point.x >= rect.x_min && point.x <= rect.x_max
		&& point.y >= rect.y_min && point.y <= rect.y_max)
		return true;
	return false;
}


static inline bool
polygon_contains(const IE::polygon& poly, const IE::point& point)
{
	if (point.x >= poly.x_min && point.x <= poly.x_max
			&& point.y >= poly.y_min && point.y <= poly.y_max)
		return true;
	return false;
}


static inline GFX::rect
center_rect_in_rect(const GFX::rect& toCenter, const GFX::rect& windowRect)
{
	GFX::rect rect = toCenter;

	rect.x = (windowRect.w - toCenter.x) / 2;
	rect.y = (windowRect.h - toCenter.y) / 2;

	return rect;
}

#endif // __RECTUTILS_H
