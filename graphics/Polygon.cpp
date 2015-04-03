#include "Bitmap.h"
#include "Polygon.h"
#include "RectUtils.h"

#include <assert.h>
#include <stdexcept>
#include <stdlib.h>

Polygon::Polygon()
	:
	fPoints(NULL),
	fCount(0),
	fFlags(0)
{
}


Polygon::Polygon(const Polygon& other)
	:
	fPoints(NULL)
{
	*this = other;
}


Polygon::~Polygon()
{
	free(fPoints);
}


GFX::rect
Polygon::Frame() const
{
	return fFrame;
}


void
Polygon::SetFrame(const int16 left, const int16 right,
			const int16 top, const int16 bottom)
{
	fFrame.x = left;
	fFrame.y = top;
	fFrame.w = right - left;
	fFrame.h = bottom - top;
}


uint8
Polygon::Flags() const
{
	return fFlags;
}


void
Polygon::SetFlags(const uint8 flags)
{
	fFlags = flags;
}


bool
Polygon::AddPoints(IE::point *points, int32 num)
{
	void *newPoints = realloc(fPoints, (fCount + num) * sizeof(IE::point));
	if (newPoints == NULL)
		return false;

	fPoints = (IE::point*)newPoints;
	memcpy(fPoints + fCount, points, num * sizeof(IE::point));
	fCount += num;

	// Recalculate Bounds
	// Seems like some polygons in some games have a wrong bounding box.
	sint16 left = fFrame.x;
	sint16 top = fFrame.y;
	sint16 right = fFrame.x + fFrame.w;
	sint16 bottom = fFrame.y + fFrame.h;
	for (int32 v = 0; v < fCount; v++) {
		const IE::point& point = fPoints[v];
		if (point.x < left)
			left = point.x;
		else if (point.x > right)
			right = point.x;
		if (point.y < top)
			top = point.y;
		else if (point.y > bottom)
			bottom = point.y;
	}

	fFrame.x = left;
	fFrame.y = top;
	fFrame.w = right - left;
	fFrame.h = bottom - top;

	return true;
}


int32
Polygon::CountPoints() const
{
	return fCount;
}


IE::point*
Polygon::Points() const
{
	return fPoints;
}


IE::point
Polygon::PointAt(int32 i) const
{
	if (i < 0 || i >= fCount)
		throw std::range_error("Polygon::PointAt()");

	return fPoints[i];
}


void
Polygon::OffsetBy(int32 x, int32 y)
{
	for (int32 v = 0; v < fCount; v++)
		fPoints[v] = offset_point(fPoints[v], x, y);

	fFrame = offset_rect(fFrame, x, y);
}


bool
Polygon::Contains(const IE::point& point) const
{
	return rect_contains(fFrame, point);
}


Polygon&
Polygon::operator=(const Polygon& polygon)
{
	fPoints = NULL;
	fCount = polygon.fCount;
	fFlags = polygon.fFlags;
	if (fCount > 0) {
		fPoints = (IE::point*)malloc(fCount * sizeof(IE::point));
		memcpy(fPoints, polygon.fPoints, fCount * sizeof(IE::point));
	}
	fFrame = polygon.fFrame;
	return *this;
}


void
Polygon::Print() const
{
	if (fPoints == NULL)
		return;

	printf("Vertices:\n");
	for (int32 v = 0; v < fCount; v++) {
		printf("\tvertex %d: x: %d, y: %d\n",
			v, fPoints[v].x, fPoints[v].y);
	}

	printf("Bounding box:\n");
	printf("\t%d - %d\n", fFrame.x, fFrame.x + fFrame.w);
	printf("\t%d - %d\n", fFrame.y, fFrame.y + fFrame.h);
	printf("Flags: 0x%x\n", fFlags);
}
