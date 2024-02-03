#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <deque>
#include <vector>


struct point_node;

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);
typedef std::deque<point_node*> NodeList;
typedef std::deque<IE::point> PointList;


class Bitmap;
class PathFinder;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int16 step = kStep, test_function func = IsPassableDefault, bool checkNeighbors = false);
	~PathFinder();

	IE::point SetPoints(const IE::point& start, const IE::point& end);
	void GetPoints(std::vector<IE::point>& points) const;

	IE::point NextWayPoint(const int& step = 1);
	bool IsEmpty() const;
	PointList* Points() const;

	void SetDebug(debug_function callback);

	bool GenerateNodes(Bitmap* searchMap);
	bool GeneratePath(const IE::point& start, const IE::point& end);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

private:
	int16 fStep;
	PointList* fPoints;
	NodeList* fClosedNodeList;
	test_function fTestFunction;
	bool fCheckNeighbors;

	debug_function fDebugFunction;

	bool IsCloseEnough(const IE::point& point, const IE::point& goal) const;
	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;

	bool _CreateDirectPath(const IE::point& start, const IE::point& end);
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

};


#endif // __PATHFIND_H
