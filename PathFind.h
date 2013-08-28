#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"
#include "list"

struct point_node;
class PathFinder {
public:
	PathFinder();
	void SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();

	static bool IsPassable(const IE::point& point);

private:
	std::list<IE::point> fPoints;
	std::list<IE::point>::iterator fIterator;

	std::list<point_node*> fOpenList;
	std::list<point_node*> fClosedList;

	void _GeneratePath(const IE::point& start, const IE::point& end);
	void _AddPassableAdiacentPoints(point_node& node);
	void _AddIfPassable(const IE::point& point, point_node& node);
	point_node* _ChooseCheapestNode(const IE::point& end);
	uint32 _HeuristicDistance(const IE::point& start, const IE::point& end);

	bool _IsPointInOpenList(const IE::point& point);
};
#endif // __PATHFIND_H
