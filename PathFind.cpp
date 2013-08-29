#include "PathFind.h"
#include "Room.h"
#include "RectUtils.h"
#include "GraphicsEngine.h"

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

const static int kStep = 4;


static bool
PointNear(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= kStep)
		&& (std::abs(pointA.y - pointB.y) <= kStep);
}


PathFinder::PathFinder()
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


IE::point
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	fPoints.clear();

	IE::point reachableEnd = end;
	if (!IsPassable(end))
		reachableEnd = _FindNearestReachablePoint(end);

	point_node* parent = new point_node(start, NULL, 0);
	fOpenList.push_back(parent);
	_AddPassableAdiacentPoints(*parent);
	fClosedList.remove(parent);

	uint32 tries = 1000;
	bool notFound = false;
	for (;;) {
		point_node* cheapestNode = _ChooseCheapestNode(end);
		assert(cheapestNode != NULL);

		fOpenList.remove(cheapestNode);
		fClosedList.push_back(cheapestNode);
#if 1
		IE::point offsettedPoint = offset_point(cheapestNode->point,
										-Room::Get()->AreaOffset().x,
										-Room::Get()->AreaOffset().y);

		GraphicsEngine::Get()->ScreenBitmap()->PutPixel(offsettedPoint.x,
				offsettedPoint.y, 4000);
		GraphicsEngine::Get()->Flip();
#endif
		if (PointNear(cheapestNode->point, end) || fOpenList.empty())
			break;

		_AddPassableAdiacentPoints(*cheapestNode);
		if (--tries == 0) {
			notFound = true;
			break;
		}
	}

	if (notFound) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		std::cout << "Path not found" << std::endl;
		std::list<point_node*>::const_iterator i;
		for (i = fClosedList.begin(); i != fClosedList.end(); i++){
			delete *i;
		}

		for (i = fOpenList.begin(); i != fOpenList.end(); i++) {
			delete *i;
		}

		return start;
	}

	std::list<point_node*>::reverse_iterator r = fClosedList.rbegin();
	point_node* walkNode = *r;
	for (;;) {
		fPoints.push_front(walkNode->point);
		const point_node* parent = walkNode->parent;
		if (parent == NULL)
			break;
		walkNode = const_cast<point_node*>(parent);
	}

	std::list<point_node*>::const_iterator i;
	for (i = fClosedList.begin(); i != fClosedList.end(); i++)
		delete *i;

	for (i = fOpenList.begin(); i != fOpenList.end(); i++)
		delete *i;

	fOpenList.clear();
	fClosedList.clear();

	std::cout << "Path:" << std::endl;
	std::list<IE::point>::const_iterator p;
	for (p = fPoints.begin(); p != fPoints.end(); p++)
		std::cout << "\t(" << (*p).x << ", " << (*p).y << ")" << std::endl;

	assert (PointNear(fPoints.back(), end));
	return fPoints.back();
}


void
PathFinder::_AddPassableAdiacentPoints(const point_node& current)
{
	IE::point newPoint = current.point;
	newPoint.x += kStep;
	_AddIfPassable(newPoint, current);
	newPoint.y += kStep;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.x -= kStep;
	_AddIfPassable(newPoint, current);
	newPoint.y += kStep;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.y += kStep;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.y -= kStep;
	_AddIfPassable(newPoint, current);
	newPoint.x -= kStep;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.x += kStep;
	newPoint.y -= kStep;
	_AddIfPassable(newPoint, current);
}




static inline const uint32
CalculateCost(const IE::point& pointA, const IE::point& pointB)
{
	return (pointA.x == pointB.x)
			|| (pointA.y == pointB.y) ? 10 : 14;
}


void
PathFinder::_AddIfPassable(const IE::point& point, const point_node& current)
{
	if (point.x < 0 || point.y < 0 || !IsPassable(point))
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
		const int cost = CalculateCost(point, current.point);
		fOpenList.push_back(new point_node(point, &current,
								current.cost + cost));
	}
}


point_node*
PathFinder::_ChooseCheapestNode(const IE::point& end)
{
	std::list<point_node*>::const_iterator i;
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (i = fOpenList.begin(); i != fOpenList.end(); i++) {
		const point_node* node = *i;
		uint32 totalCost = _HeuristicDistance(node->point, end) + node->cost;
		if (totalCost < minCost) {
			result = *i;
			minCost = totalCost;
		}
	}

	return result;
}


uint32
PathFinder::_HeuristicDistance(const IE::point& start, const IE::point& end)
{
	// Manhattan method
	return (int32)((std::abs(end.x - start.x) + std::abs(end.y - start.y)) * 10);
}


IE::point
PathFinder::_FindNearestReachablePoint(const IE::point& point)
{
	return point;
}
