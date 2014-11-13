#include "PathFind.h"
#include "Room.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
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


bool
PathFinder::IsPassable(const IE::point& point) const
{
	return fTestFunction(point);
}


/* static */
bool
PathFinder::IsStraightlyReachable(const IE::point& start, const IE::point& end)
{
	PathFinder testPath(1);

	if (!testPath.IsPassable(start) || !testPath.IsPassable(end))
		return false;

	return testPath._CreateDirectPath(start, end) == end;
}


IE::point
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	fPoints.clear();

	IE::point maxReachableDirectly = start;//_CreateDirectPath(start, end);

	if (PointSufficientlyClose(maxReachableDirectly, end)
			|| !IsPassable(end))
		return maxReachableDirectly;

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	fOpenList.push_back(currentNode);

	uint32 tries = 4000;
	bool notFound = false;
	for (;;) {
		_AddPassableAdiacentPoints(*currentNode);
		fOpenList.remove(currentNode);
		fClosedList.push_back(currentNode);

		if (PointSufficientlyClose(currentNode->point, end))
			break;

		currentNode = _ChooseCheapestNode(end);
		if (currentNode == NULL || --tries == 0) {
			notFound = true;
			break;
		}
	}

	if (notFound) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		std::cout << "Path not found" << std::endl;

		_EmptyLists();

		return start;
	}

	std::list<point_node*>::reverse_iterator r = fClosedList.rbegin();
	point_node* walkNode = *r;
	for (;;) {
		//fPoints.insert(directRouteEnd, walkNode->point);
		//directRouteEnd--;
		fPoints.push_front(walkNode->point);
		const point_node* parent = walkNode->parent;
		if (parent == NULL)
			break;
		walkNode = const_cast<point_node*>(parent);
	}

	_EmptyLists();
/*
	std::cout << "Path from (" << start.x << ", " << start.y << ") to (";
	std::cout << end.x << ", " << end.y << "): " << std::endl;
	std::list<IE::point>::const_iterator p;
	for (p = fPoints.begin(); p != fPoints.end(); p++)
		std::cout << "\t(" << (*p).x << ", " << (*p).y << ")" << std::endl;
*/
	assert (PointSufficientlyClose(fPoints.back(), end));
	return fPoints.back();
}


void
PathFinder::_AddPassableAdiacentPoints(const point_node& current)
{
	_AddIfPassable(offset_point(current.point, fStep, 0), current);
	_AddIfPassable(offset_point(current.point, fStep, fStep), current);
	_AddIfPassable(offset_point(current.point, 0, fStep), current);
	_AddIfPassable(offset_point(current.point, -fStep, fStep), current);
	_AddIfPassable(offset_point(current.point, -fStep, 0), current);
	_AddIfPassable(offset_point(current.point, -fStep, -fStep), current);
	_AddIfPassable(offset_point(current.point, fStep, -fStep), current);
	_AddIfPassable(offset_point(current.point, 0, -fStep), current);
}


static inline const uint32
CalculateCost(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) < PathFinder::kStep)
			|| (std::abs(pointA.y - pointB.y) < PathFinder::kStep) ? 10 : 14;
}


void
PathFinder::_AddIfPassable(const IE::point& point, const point_node& current)
{
	if (point.x < 0 || point.y < 0
			|| !IsPassable(point))
		return;

	// Check if point is in closed list
	std::list<point_node*>::const_iterator i =
				std::find_if(fClosedList.begin(), fClosedList.end(),
							FindPoint(point));
	if (i != fClosedList.end()) {
		const int newCost = CalculateCost(current.point, (*i)->point) + current.cost;
		if (newCost < (*i)->cost) {
			(*i)->parent = &current;
			(*i)->cost = newCost;
		}
		return;
	}

	i = std::find_if(fOpenList.begin(), fOpenList.end(), FindPoint(point));
	if (i != fOpenList.end()) {
		// Point is already on the open list.
		// Check if getting through the point from this point
		// is cheaper. If so, set this as parent.
		const int newCost = CalculateCost(current.point, (*i)->point) + current.cost;
		if (newCost < (*i)->cost) {
			(*i)->parent = &current;
			(*i)->cost = newCost;
		}
	} else {
		const int cost = CalculateCost(point, current.point) + current.cost;
		fOpenList.push_back(new point_node(point, &current, cost));
	}
}


static inline uint32
HeuristicDistance(const IE::point& start, const IE::point& end)
{
	// Manhattan method
	return 10 * (int32)((std::abs(end.x - start.x) << 2)
			+ (std::abs(end.y - start.y) << 2));
}


point_node*
PathFinder::_ChooseCheapestNode(const IE::point& end)
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (std::list<point_node*>::const_iterator i = fOpenList.begin();
			i != fOpenList.end(); i++) {
		const point_node* node = *i;
		const uint32 totalCost = HeuristicDistance(node->point, end)
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
	int lgStep = lgDelta > 0 ? fStep : -fStep;
	lgDelta = std::abs(lgDelta);
	int shStep = shDelta > 0 ? fStep : -fStep;
	shDelta = std::abs(shDelta);
	if (shDelta < lgDelta) {
		cycle = lgDelta >> 1;
		while (std::abs(point.x - end.x) > fStep) {
			if (!IsPassable(point))
				return fPoints.back();
			fPoints.push_back(point);
			cycle += shDelta;
			if (cycle > lgDelta) {
				cycle -= lgDelta;
				point.y += shStep;
			}
			point.x += lgStep;
		}
		if (!IsPassable(point))
			return fPoints.back();

		fPoints.push_back(point);
	}
	cycle = shDelta >> 1;
	while (std::abs(point.y - end.y) > fStep) {
		if (!IsPassable(point))
			return fPoints.back();
		fPoints.push_back(point);
		cycle += lgDelta;
		if (cycle > shDelta) {
			cycle -= shDelta;
			point.x += lgStep;
		}
		point.y += shStep;
	}

	if (IsPassable(end))
		fPoints.push_back(end);

	return fPoints.back();
}


void
PathFinder::_EmptyLists()
{
	std::list<point_node*>::const_iterator i;
	for (i = fClosedList.begin(); i != fClosedList.end(); i++)
		delete *i;
	fClosedList.clear();

	for (i = fOpenList.begin(); i != fOpenList.end(); i++)
		delete *i;
	fOpenList.clear();
}
