#ifndef __RECTUTILS_H
#define __RECTUTILS_H

#include "Bitmap.h"
#include "GraphicsDefs.h"
#include "IETypes.h"


static inline GFX::rect
offset_rect(const GFX::rect &rect, sint16 x, sint16 y)
{
	GFX::rect newRect = rect;
	newRect.x += x;
	newRect.y += y;
	return newRect;
}


static inline IE::rect
offset_rect(const IE::rect &rect, sint16 x, sint16 y)
{
	IE::rect newRect = rect;
	newRect.x_min += x;
	newRect.y_min += y;
	newRect.x_max += x;
	newRect.y_max += y;
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


static inline GFX::point
offset_point(const GFX::point &point, sint16 x, sint16 y)
{
	GFX::point newPoint = point;
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
rect_contains(const GFX::rect& rect, const int16 x, const int16 y)
{
	if (x >= rect.x && x <= rect.x + rect.w
		&& y >= rect.y && y <= rect.y + rect.h)
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
	if (point.x >= poly.left && point.x <= poly.right
			&& point.y >= poly.top && point.y <= poly.bottom)
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


static inline GFX::rect
rect_to_gfx_rect(const IE::rect& rect)
{
	GFX::rect gfxRect(
			rect.x_min,
			rect.y_min,
			rect.Width(),
			rect.Height()
	);
	return gfxRect;
}


static inline IE::rect
gfx_rect_to_rect(const GFX::rect& rect)
{
	IE::rect IERect = {
			rect.x,
			rect.y,
			(int16)(rect.x + rect.w),
			(int16)(rect.y + rect.h)
	};
	return IERect;
}

#endif // __RECTUTILS_H
