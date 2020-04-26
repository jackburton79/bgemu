#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <list>

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);

typedef std::list<IE::point> PointList;

class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	IE::point SetPoints(const IE::point& start, const IE::point& end);

	IE::point NextWayPoint();
	bool IsEmpty() const;

	void GetPoints(PointList points) const;

	void SetDebug(debug_function callback);
	
	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;
private:
	PointList fPoints;
	IE::point fGoal;

	test_function fTestFunction;
	int fStep;

	debug_function fDebugFunction;
	
	IE::point _GeneratePath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddNeighbors(const IE::point& current, PointList& openList);
	IE::point _GetCheapestNode(PointList& list, const IE::point& end);

	IE::point _CreateDirectPath(const IE::point&, const IE::point& point);
	void _ReconstructPath(PointList& openList);
};


#endif // __PATHFIND_H
