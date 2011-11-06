#ifndef __RECTUTILS_H
#define __RECTUTILS_H

static inline SDL_Rect
offset_rect(const SDL_Rect &rect, int32 x, int32 y)
{
	SDL_Rect newRect = rect;
	newRect.x += x;
	newRect.y += y;
	return newRect;
}


static inline bool
rects_intersect(const SDL_Rect &rectA, const SDL_Rect &rectB)
{
	return !(rectA.x > rectB.x + rectB.w
			|| rectA.y > rectB.y + rectB.h
			|| rectA.x + rectA.w < rectB.x
			|| rectA.y + rectA.h < rectB.y);
}

#endif // __RECTUTILS_H
