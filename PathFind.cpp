#include "PathFind.h"
#include "Utils.h"

PathFinder::PathFinder()
{

}


void
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	_GeneratePath(start, end);
	fIterator = fPoints.begin();
}


IE::point
PathFinder::NextWayPoint()
{
	std::list<IE::point>::iterator i = fIterator;
	if (i == fPoints.end())
		throw -1;

	return *fIterator++;
}


void
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	fPoints.clear();

	IE::point point = start;
	int cycle;
	int lg_delta = end.x - point.x;
	int sh_delta = end.y - point.y;
	int lg_step = SGN(lg_delta);
	lg_delta = ABS(lg_delta);
	int sh_step = SGN(sh_delta);
	sh_delta = ABS(sh_delta);
	if (sh_delta < lg_delta) {
		cycle = lg_delta >> 1;
		while (point.x != end.x) {
			fPoints.push_back(point);
			cycle += sh_delta;
			if (cycle > lg_delta) {
				cycle -= lg_delta;
				point.y += sh_step;
			}
			point.x += lg_step;
		}
		fPoints.push_back(point);
	}
	cycle = sh_delta >> 1;
	while (point.y != end.y) {
		fPoints.push_back(point);
		cycle += lg_delta;
		if (cycle > sh_delta) {
			cycle -= sh_delta;
			point.x += lg_step;
		}
		point.y += sh_step;
	}
}
