#include "PathFind.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <limits.h>

#define PATHFIND_MAX_TRIES 5000

const int kMovementCost = 2;
const int kDiagMovementCost = 3;

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


struct FindPoint {
	FindPoint(const IE::point& point)
		: toFind(point) {};

	bool operator() (const point_node* node) const {
		return node->point == toFind;
	};
	const IE::point& toFind;
};


static inline uint32
Distance(const IE::point& start, const IE::point& end)
{
#if 1
	// Manhattan method
	uint32 distance = (uint32)(((std::abs(end.x - start.x)) + 
		std::abs(end.y - start.y)));
#else
	// Movement distance
	uint32 distance = (uint32)std::max((std::abs(end.x - start.x)),
		std::abs(end.y - start.y));
#endif
	// We multiply by 10 since minimum movement cost is 10
	return distance * kMovementCost;
}


PathFinder::PathFinder(int step, test_function testFunc)
	:
	fStep(step),
	fPoints(NULL),
	fClosedNodeList(NULL),
	fTestFunction(testFunc),
	fDebugFunction(NULL)
{
}


PathFinder::~PathFinder()
{
	delete fPoints;
	delete fClosedNodeList;
}


IE::point
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	// TODO: Return a bool here
	if (!_GeneratePath(start, end))
		throw std::runtime_error("PathFinder::SetPoints: cannot create path");
	return fPoints->back();
}


void
PathFinder::SetDebug(debug_function callback)
{
	fDebugFunction = callback;
}


IE::point
PathFinder::NextWayPoint()
{
	assert(fPoints != NULL);
	assert(!fPoints->empty());

	IE::point point = fPoints->front();
	fPoints->pop_front();
	return point;
}


bool
PathFinder::IsEmpty() const
{
	return fPoints == NULL || fPoints->empty();
}


void
PathFinder::GetPoints(PointList& points) const
{
	if (fPoints != NULL)
		points = *fPoints;
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


bool
PathFinder::IsCloseEnough(const IE::point& point, const IE::point& goal)
{
#if 0
	return pointA == pointB;
#else

	return (std::abs(point.x - goal.x) <= fStep)
		&& (std::abs(point.y - goal.y) <= fStep);
#endif
}


static void
EmptyClosedList(ClosedNodeList*& pointList)
{
	ClosedNodeList::iterator i;
	for (i = pointList->begin(); i != pointList->end(); i++) {
		delete (*i);
	}
	delete pointList;
	pointList = NULL;
}


bool
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	delete fPoints;
	fPoints = new PointList;
	
	if (!_IsPassable(end))
		return false;

	IE::point maxReachableDirectly = start;
	if (IsCloseEnough(maxReachableDirectly, end))
		return true;

	fClosedNodeList = new ClosedNodeList();

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	currentNode->cost_to_goal = Distance(currentNode->point, end)
		+ currentNode->cost;
	currentNode->open = true;
	fClosedNodeList->push_back(currentNode);

	uint32 tries = PATHFIND_MAX_TRIES;
	bool found = false;
	while ((currentNode = _GetCheapestNode()) != NULL) {
		if (IsCloseEnough(currentNode->point, end)) {
			found = true;
			break;
		}

		if (--tries == 0)
			break;

		currentNode->open = false;
		_AddNeighbors(*currentNode, end);
		
		if (fDebugFunction != NULL)
			fDebugFunction(currentNode->point);
	}

	if (!found) {
		// TODO: failed to create path: destination is unreachable.
		EmptyClosedList(fClosedNodeList);
		return false;
	}

	point_node* last = fClosedNodeList->back();
	_ReconstructPath(last);
	EmptyClosedList(fClosedNodeList);
	
	// remove the "current" position, it's useless
	fPoints->erase(fPoints->begin());
	
	return true;
}


uint32
PathFinder::MovementCost(const IE::point& pointA, const IE::point& pointB) const
{
	// Movement cost. Bigger when moving diagonally
	return (std::abs(pointA.x - pointB.x) < fStep)
			|| (std::abs(pointA.y - pointB.y) < fStep) ?
				 kMovementCost : kDiagMovementCost;
}


bool
PathFinder::_IsPassable(const IE::point& point) const
{
	return fTestFunction(point);
}


bool
PathFinder::_IsReachable(const IE::point& current, const IE::point& point) const
{
#if 1
	return _IsPassable(point);
#else
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
	// Check if diagonal movement is possible.
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
#endif
}


void
PathFinder::_AddIfPassable(const IE::point& point,
		const point_node& current,
		const IE::point& goal)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list. If so, update it.
	ClosedNodeList::const_iterator i =
			std::find_if(fClosedNodeList->begin(), fClosedNodeList->end(),
							FindPoint(point));
	if (i != fClosedNodeList->end()) {
		_UpdateNodeCost(*i, current, goal);
		return;
	}

	// Otherwise, add it to the open list
	point_node* node = new point_node(point, &current, UINT_MAX);
	node->open = true;
	fClosedNodeList->push_back(node);
	_UpdateNodeCost(node, current, goal);
}


void
PathFinder::_AddNeighbors(const point_node& node,
		const IE::point& goal)
{
	const IE::point pointArray[] = {
			offset_point(node.point, -fStep, -fStep),
			offset_point(node.point, -fStep, 0),
			offset_point(node.point, -fStep, +fStep),
			offset_point(node.point, 0, -fStep),
			offset_point(node.point, 0, +fStep),
			offset_point(node.point, +fStep, -fStep),
			offset_point(node.point, +fStep, 0),
			offset_point(node.point, +fStep, +fStep)
	};
	const size_t arraySize = sizeof(pointArray) / sizeof(pointArray[0]);
	for (size_t c = 0; c < arraySize; c++)
		_AddIfPassable(pointArray[c], node, goal);
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
PathFinder::_GetCheapestNode()
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (ClosedNodeList::const_iterator i = fClosedNodeList->begin();
			i != fClosedNodeList->end(); i++) {
		point_node* node = *i;
		if (!node->open)
			continue;
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
	while (walkNode != NULL) {
		fPoints->push_front(walkNode->point);
		walkNode = const_cast<point_node*>(walkNode->parent);
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
				return fPoints->back();
			fPoints->push_back(point);
			cycle += shDelta;
			if (cycle > lgDelta) {
				cycle -= lgDelta;
				point.y += shStep;
			}
			point.x += lgStep;
		}
		if (!_IsPassable(point))
			return fPoints->back();

		fPoints->push_back(point);
	}
	cycle = shDelta >> 1;
	while (std::abs(point.y - end.y) > fStep) {
		if (!_IsPassable(point))
			return fPoints->back();
		fPoints->push_back(point);
		cycle += lgDelta;
		if (cycle > shDelta) {
			cycle -= shDelta;
			point.x += lgStep;
		}
		point.y += shStep;
	}

	if (_IsPassable(end))
		fPoints->push_back(end);

	return fPoints->back();
}
