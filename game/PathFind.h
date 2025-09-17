#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"

#include <climits>
#include <deque>
#include <vector>


struct point_node;

typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);
typedef std::deque<point_node*> NodeList;
typedef std::deque<IE::point> PointList;



class PathNotFoundException : public std::runtime_error {
public:
	PathNotFoundException();
};


class Path {
public:
	Path();
	Path(const IE::point start, const IE::point end, test_function func);
	~Path();

	void Set(const IE::point& start, const IE::point& end, test_function func);
	void Clear();

	IE::point Start() const;
	IE::point End() const;

	void AddPoint(const IE::point& point, test_function func);

	IE::point NextStep(const int& step = 1);
	bool IsEmpty() const;
	bool IsEnd() const;
	void Rewind();

private:
	PointList* fPoints;
	PointList::iterator fIterator;
};



struct point_node {
	point_node(IE::point p, const point_node* parentNode, int nodeCost)
		:
		point(p),
		parent(parentNode),
		cost(nodeCost),
		cost_to_goal(UINT_MAX),
		open(false)
	{
	};
	const IE::point point;
	const struct point_node* parent;
	uint32 cost;
	uint32 cost_to_goal;
	bool open;
};


class Bitmap;
class PathFinder;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int16 step = kStep, test_function func = IsPassableDefault, bool checkNeighbors = false);
	~PathFinder();

	static void SetDebug(debug_function callback);

	bool GenerateNodes(Bitmap* searchMap);
	PointList GeneratePath(const IE::point& start, const IE::point& end);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsInLineOfSight(const IE::point& start, const IE::point& end);

private:
	int16 fStep;
	test_function fTestFunction;
	bool fCheckNeighbors;

	static debug_function sDebugFunction;

	IE::point HalfPoint(const IE::point& start, const IE::point& end);

	bool IsCloseEnough(const IE::point& point, const IE::point& goal) const;
	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;

	bool CreateLineOfSightPath(const IE::point& start, const IE::point& end);

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddIfPassable(const IE::point& point,
			const point_node& current,
			const IE::point& goal,
			NodeList* nodeList);
	void _UpdateNodeCost(point_node* node, const point_node& current,
			const IE::point& goal) const;
};


#endif // __PATHFIND_H
