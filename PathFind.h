#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"
#include "list"

struct point_node;
class PathFinder {
public:
	PathFinder(bool ignoreUnpassable = false);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	static bool IsPassable(const IE::point& point);

private:
	std::list<IE::point> fPoints;

	std::list<point_node*> fOpenList;
	std::list<point_node*> fClosedList;

	bool fIgnoreUnpassable;

	IE::point _GeneratePath(const IE::point& start, const IE::point& end);
	void _AddPassableAdiacentPoints(const point_node& node);
	void _AddIfPassable(const IE::point& point, const point_node& node);
	point_node* _ChooseCheapestNode(const IE::point& end);
	uint32 _HeuristicDistance(const IE::point& start, const IE::point& end);

	IE::point _FindNearestReachablePoint(const IE::point&, const IE::point& point);
	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
};
#endif // __PATHFIND_H
