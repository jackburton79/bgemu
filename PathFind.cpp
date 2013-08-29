#include "PathFind.h"
#include "Room.h"

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
	IE::point point;
	const struct point_node* parent;
	int cost;
};


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
	std::cout << std::dec;
	std::cout << "GeneratePath: (" << start.x << ", " << start.y << ") - (";
	std::cout << end.x << ", " << end.y << ")" << std::endl;

	fPoints.clear();

	point_node* parent = new point_node(start, NULL, 0);

	fOpenList.push_back(parent);
	_AddPassableAdiacentPoints(*parent);
	fOpenList.remove(parent);
	fClosedList.push_back(parent);

	uint32 tries = 3000;
	bool notFound = false;
	for (;;) {
		point_node* cheapestNode = _ChooseCheapestNode(end);
		assert(cheapestNode != NULL);
		std::cout << "Cheapest Point: (" << cheapestNode->point.x << ", ";
		std::cout << cheapestNode->point.y << "): cost ";
		std::cout << cheapestNode->cost << std::endl;
		fOpenList.remove(cheapestNode);
		fClosedList.push_back(cheapestNode);
		cheapestNode->parent = parent;
		if (cheapestNode->point == end || fOpenList.empty())
			break;
		_AddPassableAdiacentPoints(*cheapestNode);
		if (--tries == 0) {
			notFound = true;
			break;
		}
		parent = cheapestNode;
	}

	if (notFound) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		std::cout << "Path not found" << std::endl;
		std::list<point_node*>::const_iterator i;
		for (i = fClosedList.begin(); i != fClosedList.end(); i++)
			delete *i;
		for (i = fOpenList.begin(); i != fOpenList.end(); i++)
			delete *i;

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
/*
	std::cout << "real end: " << end.x << ", " << end.y << std::endl;
	std::cout << "reachable: " << fPoints.back().x << ", " << fPoints.front().y << std::endl;
	std::flush(std::cout);
*/
	std::cout << "Path:" << std::endl;
	std::list<IE::point>::const_iterator p;
	for (p = fPoints.begin(); p != fPoints.end(); p++)
		std::cout << "\t(" << (*p).x << ", " << (*p).y << ")" << std::endl;


	assert (fPoints.back() == end);
	return fPoints.back();
}


void
PathFinder::_AddPassableAdiacentPoints(point_node& current)
{
	IE::point newPoint = current.point;
	newPoint.x += 1;
	_AddIfPassable(newPoint, current);
	newPoint.y += 1;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.x -= 1;
	_AddIfPassable(newPoint, current);
	newPoint.y += 1;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.y += 1;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.y -= 1;
	_AddIfPassable(newPoint, current);
	newPoint.x -= 1;
	_AddIfPassable(newPoint, current);

	newPoint = current.point;
	newPoint.x += 1;
	newPoint.y -= 1;
	_AddIfPassable(newPoint, current);
}


struct FindPoint {
	FindPoint(const IE::point& point)
		: toFind(point) {};

	bool operator() (const point_node* node) {
		return node->point == toFind;
	};
	const IE::point& toFind;
};


static inline const uint32
CalculateCost(const IE::point& pointA, const IE::point& pointB)
{
	return (pointA.x == pointB.x)
			|| (pointA.y == pointB.y) ? 10 : 14;
}


void
PathFinder::_AddIfPassable(const IE::point& point, point_node& current)
{
	if (point.x < 0 || point.y < 0 || !IsPassable(point))
		return;

	std::cout << std::dec;
		std::cout << "point (" << point.x << ", " << point.y << ") ";
	// Check if point is in closed list
	std::list<point_node*>::const_iterator iter =
				std::find_if(fClosedList.begin(), fClosedList.end(),
							FindPoint(point));
	if (iter != fClosedList.end()) {
		std::cout << "already in closed list" << std::endl;
		return;
	}

	std::list<point_node*>::const_iterator i =
			std::find_if(fOpenList.begin(), fOpenList.end(),
						FindPoint(point));
	if (i != fOpenList.end()) {
		std::cout << "already in open list.";
		// Point is already on the open list.
		// Check if getting through the point from this point
		// is cheaper. If so, set this as parent.
		const int newCost = CalculateCost(current.point, (*i)->point) + current.cost;
		//std::cout << " Old cost: " << (*i)->cost << ", new: " << newCost << std::endl;
		if (newCost < (*i)->cost) {
			std::cout << " Improved cost (" << (*i)->cost << "->";
			std::cout << newCost << ")";
			(*i)->parent = &current;
			(*i)->cost = newCost;
		}
		std::cout << std::endl;
	} else {
		std::cout << ": adding to open list (cost: ";
		const int cost = CalculateCost(point, current.point);
		point_node* newNode = new point_node(point, &current,
											current.cost + cost);
		std::cout << newNode->cost << ")" << std::endl;
		fOpenList.push_back(newNode);
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
	int32 distance = (int32)((std::abs(end.x - start.x) + std::abs(end.y - start.y)) * 10);

	std::cout << "Heuristic distance (" << start.x << ", " << start.y << ") -> ";
	std::cout << "(" << end.x << ", " << end.y << ") = " << distance << std::endl;
	return distance;
}
