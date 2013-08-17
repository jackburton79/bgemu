#include "Bitmap.h"
#include "Polygon.h"
#include "RectUtils.h"

#include <assert.h>
#include <stdlib.h>

Polygon::Polygon()
	:
	fPoints(NULL),
	fCount(0),
	fIsHole(false)
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
Polygon::SetFrame(const int16 x_min, const int16 x_max,
			const int16 y_min, const int16 y_max)
{
	fFrame.x = x_min;
	fFrame.y = y_min;
	fFrame.w = x_max;
	fFrame.h = y_max;
}


bool
Polygon::IsHole() const
{
	return fIsHole;
}


void
Polygon::SetIsHole(const bool hole)
{
	fIsHole = hole;
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
	return fPoints[i];
}


void
Polygon::OffsetBy(int32 x, int32 y)
{
	for (int32 v = 0; v < fCount; v++)
		fPoints[v] = offset_point(fPoints[v], x, y);

	fFrame = offset_rect(fFrame, x, y);
}


Polygon&
Polygon::operator=(const Polygon& polygon)
{
	fPoints = NULL;
	fCount = polygon.fCount;
	fIsHole = polygon.fIsHole;
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
	printf("\t%d - %d\n", fFrame.x, fFrame.w);
	printf("\t%d - %d\n", fFrame.y, fFrame.h);
}
