#include "PathFind.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <limits.h>

#define PATHFIND_MAX_TRIES 4000

struct FindPoint {
	FindPoint(const IE::point& point)
		: toFind(point) {};

	bool operator() (const point_node* node) {
		return node->point == toFind;
	};
	const IE::point& toFind;
};


static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= PathFinder::kStep)
		&& (std::abs(pointA.y - pointB.y) <= PathFinder::kStep);
}


static inline uint32
Distance(const IE::point& start, const IE::point& end)
{
#if 0
	// Manhattan method
	uint32 distance = (uint32)(((std::abs(end.x - start.x)) + 
		std::abs(end.y - start.y)));
#else
	// Movement distance
	uint32 distance = (uint32)std::max((std::abs(end.x - start.x)),
		std::abs(end.y - start.y));
#endif
	// We multiply by 10 since minimum movement cost is 10
	return distance * 10;
}


PathFinder::PathFinder(int step, test_function testFunc)
	:
	fTestFunction(testFunc),
	fStep(step),
	fDebugFunction(NULL)
{
}


IE::point
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	return _GeneratePath(start, end);
}


void
PathFinder::SetDebug(debug_function callback)
{
	fDebugFunction = callback;
}


IE::point
PathFinder::NextWayPoint()
{
	assert(!fPoints.empty());

	IE::point point = fPoints.front();
	fPoints.pop_front();
	return point;
}


bool
PathFinder::IsEmpty() const
{
	return fPoints.empty();
}


void
PathFinder::GetPoints(std::list<IE::point> points) const
{
	points = fPoints;
}


/* static */
bool
PathFinder::IsStraightlyReachable(const IE::point& start, const IE::point& end)
{
	PathFinder testPath(1);

	if (!testPath._IsPassable(start) || !testPath._IsPassable(end))
		return false;

	return testPath._CreateDirectPath(start, end) == end;
}


static void
EmptyList(NodeList pointList)
{
	NodeList::iterator i;
	for (i = pointList.begin(); i != pointList.end(); i++) {
		delete (*i);
	}
}


IE::point
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	fPoints.clear();

	IE::point maxReachableDirectly = start;
	if (PointSufficientlyClose(maxReachableDirectly, end)
			|| !_IsPassable(end))
		return maxReachableDirectly;

	NodeList openList;
	NodeList closedList;

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	currentNode->cost_to_goal = Distance(currentNode->point, end)
		+ currentNode->cost;
	openList.push_back(currentNode);

	uint32 tries = PATHFIND_MAX_TRIES;
	bool found = false;
	while (!openList.empty()) {
		currentNode = _GetCheapestNode(openList);
#if 0
		std::cout << "try " << tries << std::endl;
		std::cout << "cost to goal: " << currentNode->cost_to_goal << std::endl;
#endif
		if (currentNode == NULL || --tries == 0)
			break;

		if (PointSufficientlyClose(currentNode->point, end)) {
			found = true;
			break;
		}
		openList.remove(currentNode);
		_AddNeighbors(*currentNode, openList, closedList, end);
		closedList.push_back(currentNode);

		if (fDebugFunction != NULL)
			fDebugFunction(currentNode->point);
	}

	EmptyList(openList);

	if (!found) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		EmptyList(closedList);
		return start;
	}

	NodeList::reverse_iterator last = closedList.rbegin();
	_ReconstructPath(*last);
	EmptyList(closedList);
	
	// remove the "current" position, it's useless
	fPoints.erase(fPoints.begin());
	
	return fPoints.back();
}


uint32
PathFinder::MovementCost(const IE::point& pointA, const IE::point& pointB) const
{
	// Movement cost. Bigger when moving diagonally
	return (std::abs(pointA.x - pointB.x) < fStep)
			|| (std::abs(pointA.y - pointB.y) < fStep) ? 10 : 14;
}


bool
PathFinder::_IsPassable(const IE::point& point) const
{
	return fTestFunction(point);
}


bool
PathFinder::_IsReachable(const IE::point& current, const IE::point& point) const
{
	if (!_IsPassable(point))
		return false;
	
	int step = std::max(std::abs(point.x - current.x),
		std::abs(point.y - current.y));

	IE::point upperPt = current;
	upperPt.y -= step;

	IE::point leftPt = current;
	leftPt.x -= step;

	IE::point bottomPt = current;
	bottomPt.y += step;

	IE::point rightPt = current;
	rightPt.x += step;
	// Check if diagonal movement
	// four cases: NW, NE, SW, SE.
	// Example: if movement is towards NW, we check also if the N and W
	// nodes are passable 
	if (point.x < current.x) {
		if (point.y < current.y) {
			// NW
			if (!_IsPassable(upperPt) && !_IsPassable(leftPt))
				return false;
		} else if (point.y > current.y) {
			// SW
			if (!_IsPassable(bottomPt) && !_IsPassable(leftPt))
				return false;
		}
	} else if (point.x > current.x) {
		if (point.y < current.y) {
			// NE
			if (!_IsPassable(upperPt) && !_IsPassable(rightPt))
				return false;
		} else if (point.y > current.y) {
			// SE
			if (!_IsPassable(bottomPt) && !_IsPassable(rightPt))
				return false;
		}
	}

	return true;
}


void
PathFinder::_AddIfPassable(const IE::point& point,
		const point_node& current,
		NodeList& openList,
		NodeList& closedList,
		const IE::point& goal)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list
	NodeList::const_iterator i =
				std::find_if(closedList.begin(), closedList.end(),
							FindPoint(point));
	if (i != closedList.end()) {
		_UpdateNodeCost(*i, current, goal);
		return;
	}

	point_node* node = NULL;
	i = std::find_if(openList.begin(), openList.end(), FindPoint(point));
	if (i != openList.end()) {
		node = *i;
	} else {
		node = new point_node(point, &current, UINT_MAX);
		openList.push_back(node);
	}
	_UpdateNodeCost(node, current, goal);
}


void
PathFinder::_AddNeighbors(const point_node& node,
		NodeList& openList,
		NodeList& closedList,
		const IE::point& goal)
{
	for (int x = -fStep; x <= fStep; x += fStep) {
		for (int y = -fStep; y <= fStep; y += fStep) {
			if (x == 0 && y == 0)
				continue;
			_AddIfPassable(offset_point(node.point, x, y),
				node, openList, closedList, goal);
		}
	}
}


void
PathFinder::_UpdateNodeCost(point_node* node, const point_node& current, const IE::point& goal) const
{
	const uint32 newCost = MovementCost(current.point,
			node->point) + current.cost;
	if (newCost < node->cost) {
		node->parent = &current;
		node->cost = newCost;
		node->cost_to_goal = Distance(node->point, goal) + node->cost;
	}
}


point_node*
PathFinder::_GetCheapestNode(NodeList& list)
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (NodeList::const_iterator i = list.begin();
			i != list.end(); i++) {
		point_node* node = *i;
		if (node->cost_to_goal < minCost) {
			result = node;
			minCost = node->cost_to_goal;
		}
	}

	return result;
}


void
PathFinder::_ReconstructPath(point_node* goal)
{
	point_node* walkNode = goal;
	for (;;) {
		fPoints.push_front(walkNode->point);
		const point_node* parent = walkNode->parent;
		if (parent == NULL)
			break;
		walkNode = const_cast<point_node*>(parent);
	}
}


IE::point
PathFinder::_CreateDirectPath(const IE::point& start, const IE::point& end)
{
	IE::point point = start;
	int cycle;
	int lgDelta = end.x - point.x;
	int shDelta = end.y - point.y;
	int lgStep = std::signbit((float)lgDelta) ? -fStep : fStep;
	lgDelta = std::abs(lgDelta);
	int shStep = std::signbit((float)shDelta) ? -fStep : fStep;
	shDelta = std::abs(shDelta);
	if (shDelta < lgDelta) {
		cycle = lgDelta >> 1;
		while (std::abs(point.x - end.x) > fStep) {
			if (!_IsPassable(point))
				return fPoints.back();
			fPoints.push_back(point);
			cycle += shDelta;
			if (cycle > lgDelta) {
				cycle -= lgDelta;
				point.y += shStep;
			}
			point.x += lgStep;
		}
		if (!_IsPassable(point))
			return fPoints.back();

		fPoints.push_back(point);
	}
	cycle = shDelta >> 1;
	while (std::abs(point.y - end.y) > fStep) {
		if (!_IsPassable(point))
			return fPoints.back();
		fPoints.push_back(point);
		cycle += lgDelta;
		if (cycle > shDelta) {
			cycle -= shDelta;
			point.x += lgStep;
		}
		point.y += shStep;
	}

	if (_IsPassable(end))
		fPoints.push_back(end);

	return fPoints.back();
}
