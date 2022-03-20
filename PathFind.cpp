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
	IE::point point;
	struct point_node* parent;
	uint32 cost;
	uint32 cost_to_goal;
	struct point_node* next;
	struct point_node* previous;
	bool open;
};


struct point_node gNodes[80][80];
bool gChecked[80][80];
struct point_node* gFirst;
struct point_node* gLast;


static inline
void
list_push(struct point_node* node, struct point_node* previous)
{
	node->next = NULL;
	node->previous = previous;
	if (previous != NULL)
		previous->next = node;
	gLast = node;
}


static inline
void
list_remove(struct point_node*)
{

}


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
	fTestFunction(testFunc),
	fDebugFunction(NULL)
{
}


PathFinder::~PathFinder()
{
	delete fPoints;
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
PathFinder::IsCloseEnough(const IE::point& point, const IE::point& goal) const
{
#if 0
	return pointA == pointB;
#else

	return (std::abs(point.x - goal.x) <= fStep)
		&& (std::abs(point.y - goal.y) <= fStep);
#endif
}


bool
PathFinder::_GeneratePath(const IE::point& start, const IE::point& end)
{
	delete fPoints;
	fPoints = NULL;
	gFirst = NULL;
	gLast = NULL;

	if (!_IsPassable(end))
		return false;

	IE::point maxReachableDirectly = start;
	if (IsCloseEnough(maxReachableDirectly, end))
		return true;

	fPoints = new PointList;

	::memset(gChecked, 0, 60*60);

	point_node* currentNode = &gNodes[maxReachableDirectly.x / 10][maxReachableDirectly.y / 10];
	currentNode->point = maxReachableDirectly;
	currentNode->parent = NULL;
	currentNode->cost = 0;
	currentNode->cost_to_goal = Distance(currentNode->point, end) + currentNode->cost;
	currentNode->open = true;
	currentNode->previous = NULL;
	currentNode->next = NULL;
	gFirst = currentNode;	
	list_push(currentNode, NULL);
	
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

		// Add neighbors
		const IE::point pointArray[] = {
			offset_point(currentNode->point, -fStep, -fStep),
			offset_point(currentNode->point, -fStep, 0),
			offset_point(currentNode->point, -fStep, +fStep),
			offset_point(currentNode->point, 0, -fStep),
			offset_point(currentNode->point, 0, +fStep),
			offset_point(currentNode->point, +fStep, -fStep),
			offset_point(currentNode->point, +fStep, 0),
			offset_point(currentNode->point, +fStep, +fStep)
		};
		const size_t arraySize = sizeof(pointArray) / sizeof(pointArray[0]);
		for (size_t c = 0; c < arraySize; c++) {
			_AddIfPassable(pointArray[c], *currentNode, end);
		}
		
		if (fDebugFunction != NULL)
			fDebugFunction(currentNode->point);
	}

	if (!found) {
		// TODO: failed to create path: destination is unreachable.
		delete fPoints;
		fPoints = NULL;
		return false;
	}

	_ReconstructPath(gLast);
	
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
#if 0
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
		point_node& current,
		const IE::point& goal)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list. If so, update it.
	if (gChecked[point.x / 10][point.y / 10]) {
		_UpdateNodeCost(&gNodes[point.x / 10][point.y / 10], current, goal);
		return;
	}

	// Otherwise, add it to the open list
	std::cout << "point: " << (point.x / 10) << ", " << (point.y / 10) << std::endl;
	point_node* node = &gNodes[point.x / 10][point.y / 10];
	node->point = point;
	node->parent = &current;
	node->cost = UINT_MAX;
	node->open = true;
	node->previous = NULL;
	node->next = NULL;
	gChecked[point.x / 10][point.y / 10] = true;
	list_push(node, gLast);
	
	_UpdateNodeCost(node, current, goal);
}


void
PathFinder::_UpdateNodeCost(point_node* node, point_node& current, const IE::point& goal) const
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
PathFinder::_GetCheapestNode() const
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	
	point_node* i = NULL;
	for (i = gFirst; i != NULL; i = i->next) {
		point_node* node = i;
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
