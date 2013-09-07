#include "PathFind.h"
#include "Room.h"
#include "RectUtils.h"
#include "GraphicsEngine.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <limits.h>

#define PLOT_PATH 0

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


const static int kStep = 5;


static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= kStep * 2)
		&& (std::abs(pointA.y - pointB.y) <= kStep * 2);
}


PathFinder::PathFinder(bool ignoreUnpassable)
	:
	fIgnoreUnpassable(ignoreUnpassable)
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


/* static */
bool
PathFinder::IsPassable(const IE::point& point)
{
	if (fIgnoreUnpassable)
		return true;

	int32 state = Room::Get()->PointSearch(point);
	switch (state) {
		case 0:
		case 8:
		case 10:
		case 12:
		case 13:
			return false;
		default:
			return true;
	}
}


/* static */
bool
PathFinder::IsStraightlyReachable(const IE::point& start, const IE::point& end)
{
	PathFinder testPath(true);

	if (!testPath.IsPassable(start) || !testPath.IsPassable(end))
		return false;

	return testPath._CreateDirectPath(start, end) == end;
}


IE::point
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
#if PLOT_PATH
	const uint32 color = GraphicsEngine::Get()
								->ScreenBitmap()->MapColor(255, 128, 100);
#endif
	fPoints.clear();

	IE::point maxReachableDirectly = start;//_CreateDirectPath(start, end);
#if PLOT_PATH
	std::list<IE::point>::const_iterator d = fPoints.begin();
	for (; d != fPoints.end(); d++) {
		IE::point offsettedPoint = offset_point(*d,
									-Room::Get()->AreaOffset().x,
									-Room::Get()->AreaOffset().y);

		GraphicsEngine::Get()->ScreenBitmap()->PutPixel(offsettedPoint.x,
			offsettedPoint.y, color);
		GraphicsEngine::Get()->Flip();
	}
#endif
	if (PointSufficientlyClose(maxReachableDirectly, end)
			|| !IsPassable(end))
		return maxReachableDirectly;

	//std::list<IE::point>::iterator directRouteEnd = fPoints.end();
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

#if PLOT_PATH
		IE::point offsettedPoint = offset_point(currentNode->point,
										-Room::Get()->AreaOffset().x,
										-Room::Get()->AreaOffset().y);

		GraphicsEngine::Get()->ScreenBitmap()->PutPixel(offsettedPoint.x,
				offsettedPoint.y, color);
		GraphicsEngine::Get()->Flip();
#endif

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
	_AddIfPassable(offset_point(current.point, kStep, 0), current);
	_AddIfPassable(offset_point(current.point, kStep, kStep), current);
	_AddIfPassable(offset_point(current.point, 0, kStep), current);
	_AddIfPassable(offset_point(current.point, -kStep, kStep), current);
	_AddIfPassable(offset_point(current.point, -kStep, 0), current);
	_AddIfPassable(offset_point(current.point, -kStep, -kStep), current);
	_AddIfPassable(offset_point(current.point, kStep, -kStep), current);
	_AddIfPassable(offset_point(current.point, 0, -kStep), current);
}


static inline const uint32
CalculateCost(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) < kStep)
			|| (std::abs(pointA.y - pointB.y) < kStep) ? 10 : 14;
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
	int lg_delta = end.x - point.x;
	int sh_delta = end.y - point.y;
	int lg_step = lg_delta > 0 ? kStep : -kStep;
	lg_delta = ABS(lg_delta);
	int sh_step = sh_delta > 0 ? kStep : -kStep;
	sh_delta = ABS(sh_delta);
	if (sh_delta < lg_delta) {
		cycle = lg_delta >> 1;
		while (std::abs(point.x - end.x) > kStep) {
			if (!IsPassable(point))
				return fPoints.back();
			fPoints.push_back(point);
			cycle += sh_delta;
			if (cycle > lg_delta) {
				cycle -= lg_delta;
				point.y += sh_step;
			}
			point.x += lg_step;
		}
		if (!IsPassable(point))
			return fPoints.back();

		fPoints.push_back(point);
	}
	cycle = sh_delta >> 1;
	while (std::abs(point.y - end.y) > kStep) {
		if (!IsPassable(point))
			return fPoints.back();
		fPoints.push_back(point);
		cycle += lg_delta;
		if (cycle > sh_delta) {
			cycle -= sh_delta;
			point.x += lg_step;
		}
		point.y += sh_step;
	}

	if (IsPassable(point))
		fPoints.push_back(point);

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
