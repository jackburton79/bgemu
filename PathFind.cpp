#include "PathFind.h"
#include "Utils.h"

PathFinder::PathFinder()
{
}


void
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	_GeneratePath(start, end);
}


IE::point
PathFinder::NextWayPoint()
{
	std::list<IE::point>::const_iterator i = fIterator;
	if (i == fPoints.end())
		return *(--i);

	return *fIterator++;
}


void
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	fPoints.clear();

	IE::point point = start;
	int cycle;
	int lgDelta = end.x - point.x;
	int shDelta = end.y - point.y;
	int lgStep = SGN(lgDelta);
	lgDelta = ABS(lgDelta);
	int sh_step = SGN(shDelta);
	shDelta = ABS(shDelta);
	if (shDelta < lgDelta) {
		cycle = lgDelta >> 1;
		while (point.x != end.x) {
			fPoints.push_back(point);
			cycle += shDelta;
			if (cycle > lgDelta) {
				cycle -= lgDelta;
				point.y += sh_step;
			}
			point.x += lgStep;
		}
		fPoints.push_back(point);
	}
	cycle = shDelta >> 1;
	while (point.y != end.y) {
		fPoints.push_back(point);
		cycle += lgDelta;
		if (cycle > shDelta) {
			cycle -= shDelta;
			point.x += lgStep;
		}
		point.y += sh_step;
	}

	fPoints.push_back(end);

	fIterator = fPoints.begin();
}
