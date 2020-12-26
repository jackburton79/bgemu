#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <climits>
#include <list>
#include <map>

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);

struct point_node {
	point_node(IE::point p, const point_node* parentNode, int nodeCost)
		:
		point(p),
		parent(parentNode),
		cost(nodeCost),
		cost_to_goal(UINT_MAX)
	{
	};
	const IE::point point;
	const struct point_node* parent;
	uint32 cost;
	uint32 cost_to_goal;
};

typedef std::list<point_node*> NodeList;
typedef std::list<IE::point> PointList;

struct point_node;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	void GetPoints(PointList& points) const;

	void SetDebug(debug_function callback);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

	bool IsCloseEnough(const IE::point& point, const IE::point& goal);
	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;
private:
	PointList fPoints;

	test_function fTestFunction;
	int fStep;

	debug_function fDebugFunction;

	IE::point _GeneratePath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddIfPassable(const IE::point& point,
			const point_node& node,
			NodeList& openList,
			NodeList& closedList,
			const IE::point& goal);
	void _AddNeighbors(const point_node& node,
			NodeList& openList,
			NodeList& closedList, const IE::point& goal);
	void _UpdateNodeCost(point_node* node, const point_node& current,
			const IE::point& goal) const;
	point_node* _GetCheapestNode(NodeList& list);
	void _ReconstructPath(point_node* goal);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
};


#endif // __PATHFIND_H
