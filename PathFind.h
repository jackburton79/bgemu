#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <climits>
#include <deque>
#include <list>
#include <queue>
#include <map>

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);


struct point_node;

typedef std::deque<point_node*> ClosedNodeList;
typedef std::deque<IE::point> PointList;

class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	~PathFinder();

	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	void GetPoints(PointList& points) const;

	void SetDebug(debug_function callback);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

	bool IsCloseEnough(const IE::point& point, const IE::point& goal) const;
	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;

private:
	int fStep;
	PointList* fPoints;
	ClosedNodeList* fClosedNodeList;
	test_function fTestFunction;

	debug_function fDebugFunction;

	bool _GeneratePath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddIfPassable(const IE::point& point,
			const point_node& node,
			const IE::point& goal);
	void _AddNeighbors(const point_node& node,
			const IE::point& goal);
	void _UpdateNodeCost(point_node* node, const point_node& current,
			const IE::point& goal) const;
	point_node* _GetCheapestNode() const;
	void _ReconstructPath(point_node* goal);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
};


#endif // __PATHFIND_H
