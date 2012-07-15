#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"
#include "list"

class PathFinder {
public:
	PathFinder();
	void SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();

private:
	std::list<IE::point> fPoints;
	std::list<IE::point>::iterator fIterator;

	void _GeneratePath(const IE::point& start, const IE::point& end);
};
#endif // __PATHFIND_H
