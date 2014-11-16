#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <list>

typedef bool(*test_function)(const IE::point& start);

struct point_node;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	void GetPoints(std::list<IE::point> points) const;

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

private:
	std::list<IE::point> fPoints;

	test_function fTestFunction;
	int fStep;

	IE::point _GeneratePath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point);
	void _AddIfPassable(const IE::point& point,
			const point_node& node,
			std::list<point_node*>& openList,
			std::list<point_node*>& closedList);
	point_node* _GetCheapestNode(std::list<point_node*>& list,
			const IE::point& end);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
};


#endif // __PATHFIND_H
