#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"
#include "list"

struct point_node;
class PathFinder {
public:
	const static int kStep = 5;

	typedef bool(*test_function)(const IE::point& point);

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	bool IsPassable(const IE::point& point) const;

	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);
	static bool IsPassableDefault(const IE::point& point) { return true; };

private:
	std::list<IE::point> fPoints;

	std::list<point_node*> fOpenList;
	std::list<point_node*> fClosedList;

	test_function fTestFunction;
	int fStep;

	IE::point _GeneratePath(const IE::point& start, const IE::point& end);

	void _AddPassableAdiacentPoints(const point_node& node);
	void _AddIfPassable(const IE::point& point, const point_node& node);
	point_node* _ChooseCheapestNode(const IE::point& end);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);

	void _EmptyLists();
};
#endif // __PATHFIND_H
