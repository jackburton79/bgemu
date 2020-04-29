#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <list>
#include <map>

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);

typedef std::map<IE::point, uint32> PointsMap;

struct point_node;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	void GetPoints(std::list<IE::point> points) const;

	void SetDebug(debug_function callback);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;
private:
	std::list<IE::point> fPoints;

	test_function fTestFunction;
	int fStep;

	debug_function fDebugFunction;

	IE::point _GeneratePath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddIfPassable(const IE::point& point,
			const point_node& node,
			std::list<point_node*>& openList,
			std::list<point_node*>& closedList,
			const IE::point& goal);
	void _AddNeighbors(const point_node& node,
			std::list<point_node*>& openList,
			std::list<point_node*>& closedList, const IE::point& goal);
	void _UpdateNodeCost(point_node* node, const point_node& current,
			const IE::point& goal) const;
	point_node* _GetCheapestNode(std::list<point_node*>& list,
			const IE::point& end);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
};


#endif // __PATHFIND_H
