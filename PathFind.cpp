#include "PathFind.h"
#include "Room.h"

#include <algorithm>
#include <limits.h>

struct point_node {
	IE::point point;
	int cost;
	const struct point_node* parent;
};


PathFinder::PathFinder()
{
}


void
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	_GeneratePath(start, end);
}


IE::point
PathFinder::NextWayPoint()
{
	std::list<IE::point>::const_iterator i = fIterator;
	if (i == fPoints.end())
		return *(--i);

	return *fIterator++;
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


void
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	std::cout << std::dec;
	std::cout << "GeneratePath: (" << start.x << ", " << start.y << ") - (";
	std::cout << end.x << ", " << end.y << ")" << std::endl;

	fPoints.clear();

	point_node* node = new point_node;
	node->point = start;
	node->parent = NULL;
	node->cost = 0;

	fOpenList.push_back(node);
	_AddPassableAdiacentPoints(*node);
	fOpenList.remove(node);
	fClosedList.push_back(node);

	for (;;) {
		point_node* cheapestNode = _ChooseCheapestNode(end);
		std::cout << "Cheapest Point: (" << cheapestNode->point.x << ", ";
		std::cout << cheapestNode->point.y << "): cost ";
		std::cout << cheapestNode->cost << std::endl;
		fOpenList.remove(cheapestNode);
		fClosedList.push_back(cheapestNode);
		if (cheapestNode->point == end || fOpenList.empty())
			break;
		_AddPassableAdiacentPoints(*cheapestNode);
	}

	std::list<point_node*>::const_iterator i;
	for (i = fClosedList.begin(); i != fClosedList.end(); i++) {
		fPoints.push_back((*i)->point);
		delete *i;
	}

	fOpenList.clear();
	fClosedList.clear();

	fIterator = fPoints.begin();
}


void
PathFinder::_AddPassableAdiacentPoints(point_node& node)
{
	IE::point newPoint = node.point;
	newPoint.x += 1;
	_AddIfPassable(newPoint, node);
	newPoint.y += 1;
	_AddIfPassable(newPoint, node);

	newPoint = node.point;
	newPoint.x -= 1;
	_AddIfPassable(newPoint, node);
	newPoint.y += 1;
	_AddIfPassable(newPoint, node);

	newPoint = node.point;
	newPoint.y += 1;
	_AddIfPassable(newPoint, node);

	newPoint = node.point;
	newPoint.y -= 1;
	_AddIfPassable(newPoint, node);
	newPoint.x -= 1;
	_AddIfPassable(newPoint, node);

	newPoint = node.point;
	newPoint.x += 1;
	newPoint.y -= 1;
	_AddIfPassable(newPoint, node);
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
PathFinder::_AddIfPassable(const IE::point& point, point_node& parent)
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
		std::cout << "already in open list" << std::endl;
		// Point is already on the open list.
		// Check if getting through the point from this point
		// is cheaper. If so, set this as parent.
		const int newCost = CalculateCost(parent.point, (*i)->point) + parent.cost;
		std::cout << "old cost: " << (*i)->cost << ", new: " << newCost << std::endl;
		if (newCost < (*i)->cost) {
			std::cout << "better path, updating cost..." << std::endl;
			(*i)->parent = &parent;
			(*i)->cost = newCost;
		}
	} else {
		std::cout << ": adding to open list (cost: ";
		point_node* newNode = new point_node;
		const int cost = CalculateCost(point, parent.point);
		newNode->point = point;
		newNode->cost = parent.cost + cost;
		std::cout << newNode->cost << ")" << std::endl;
		newNode->parent = &parent;
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
