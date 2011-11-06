#include "Polygon.h"

#include <assert.h>

Polygon::Polygon()
	:
	fPoints(NULL),
	fCount(0)
{
}


Polygon::~Polygon()
{
	free(fPoints);
}


SDL_Rect
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
Polygon::AddPoints(point *points, int32 num)
{
	void *newPoints = realloc(fPoints, (fCount + num) * sizeof(point));
	if (newPoints == NULL)
		return false;

	fPoints = (point*)newPoints;
	memcpy(fPoints + fCount, points, num * sizeof(point));
	fCount += num;

	return true;
}


int32
Polygon::CountPoints() const
{
	return fCount;
}


point *
Polygon::Points() const
{
	return fPoints;
}


point
Polygon::PointAt(int32 i) const
{
	return fPoints[i];
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
