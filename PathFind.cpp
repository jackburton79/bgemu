#include "PathFind.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <limits.h>


struct point_node {
	point_node(IE::point p, const point_node* parentNode, int nodeCost)
		:
		point(p),
		parent(parentNode),
		cost(nodeCost)
	{
	};
	const IE::point point;
	const struct point_node* parent;
	int cost;
};


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
	return (std::abs(pointA.x - pointB.x) <= PathFinder::kStep * 2)
		&& (std::abs(pointA.y - pointB.y) <= PathFinder::kStep * 2);
}


PathFinder::PathFinder(int step, test_function testFunc)
	:
	fTestFunction(testFunc),
	fStep(step)
{
}


IE::point
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	return _GeneratePath(start, end);
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


bool
PathFinder::_IsPassable(const IE::point& point)
{
	return fTestFunction(point);
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
EmptyList(std::list<point_node*> pointList)
{
	std::list<point_node*>::iterator i;
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

	std::list<point_node*> openList;
	std::list<point_node*> closedList;

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	openList.push_back(currentNode);

	uint32 tries = 4000;
	bool notFound = false;
	for (;;) {
		// If adiacent nodes are passable, add them to the list
		_AddIfPassable(offset_point(currentNode->point, fStep, 0), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, fStep, fStep), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, 0, fStep), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, -fStep, fStep), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, -fStep, 0), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, -fStep, -fStep), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, fStep, -fStep), *currentNode, openList, closedList);
		_AddIfPassable(offset_point(currentNode->point, 0, -fStep), *currentNode, openList, closedList);

		openList.remove(currentNode);
		closedList.push_back(currentNode);

		if (PointSufficientlyClose(currentNode->point, end))
			break;

		currentNode = _GetCheapestNode(openList, end);
		if (currentNode == NULL || --tries == 0) {
			notFound = true;
			break;
		}
	}

	if (notFound) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		std::cout << "Path not found" << std::endl;

		EmptyList(closedList);
		EmptyList(openList);
		return start;
	}

	EmptyList(openList);
	std::list<point_node*>::reverse_iterator r = closedList.rbegin();
	point_node* walkNode = *r;
	for (;;) {
		fPoints.push_front(walkNode->point);
		const point_node* parent = walkNode->parent;
		if (parent == NULL)
			break;
		walkNode = const_cast<point_node*>(parent);
	}

	EmptyList(closedList);
	assert (PointSufficientlyClose(fPoints.back(), end));
	
	// remove the "current" position, it's useless
	fPoints.erase(fPoints.begin());
	
	return fPoints.back();
}


static inline const uint32
MovementCost(const IE::point& pointA, const IE::point& pointB)
{
	// Movement cost. Bigger when moving diagonally
	return (std::abs(pointA.x - pointB.x) < PathFinder::kStep)
			|| (std::abs(pointA.y - pointB.y) < PathFinder::kStep) ? 10 : 14;
}


void
PathFinder::_AddIfPassable(const IE::point& point,
		const point_node& current,
		std::list<point_node*>& openList,
		std::list<point_node*>& closedList)
{
	if (point.x < 0 || point.y < 0
			|| !_IsPassable(point))
		return;

	// Check if point is in closed list
	std::list<point_node*>::const_iterator i =
				std::find_if(closedList.begin(), closedList.end(),
							FindPoint(point));
	if (i != closedList.end()) {
		point_node* node = *i;
		const int newCost = MovementCost(current.point,
				node->point) + current.cost;
		if (newCost < node->cost) {
			node->parent = &current;
			node->cost = newCost;
		}
		return;
	}

	i = std::find_if(openList.begin(), openList.end(), FindPoint(point));
	if (i != openList.end()) {
		point_node* node = *i;
		// Point is already on the open list.
		// Check if getting through the point from this point
		// is cheaper. If so, set this as parent.
		const int newCost = MovementCost(current.point,
				node->point) + current.cost;
		if (newCost < node->cost) {
			node->parent = &current;
			node->cost = newCost;
		}
	} else {
		const int cost = MovementCost(point, current.point) + current.cost;
		point_node* newNode = new point_node(point, &current, cost);
		openList.push_back(newNode);
	}
}


static inline uint32
Distance(const IE::point& start, const IE::point& end)
{
	// Manhattan method
	uint32 distance = (uint32)((std::abs(end.x - start.x))
		+ (std::abs(end.y - start.y)));
	// We multiply by 10 since minimum movement cost is 10
	return distance * 10;
}


point_node*
PathFinder::_GetCheapestNode(std::list<point_node*>& list,
		const IE::point& end)
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (std::list<point_node*>::const_iterator i = list.begin();
			i != list.end(); i++) {
		const point_node* node = *i;
		const uint32 totalCost = Distance(node->point, end)
										+ node->cost;
		if (totalCost < minCost) {
			result = *i;
			minCost = totalCost;
		}
	}

	return result;
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
