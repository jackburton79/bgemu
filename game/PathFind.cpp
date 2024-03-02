#include "PathFind.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <list>
#include <map>
#include <queue>

#include <memory>

#include "Bitmap.h"
#include "Utils.h"

#define PATHFIND_MAX_TRIES 2000

const int kMovementCost = 2;
const int kDiagMovementCost = 3;

class ClosedNodes {
public:
	ClosedNodes();
	~ClosedNodes();
	point_node* GetCheapestNode() const;

	NodeList* nodelist;
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
PointDistance(const IE::point& start, const IE::point& end)
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
	return distance;
}


static inline uint32
Distance(const IE::point& start, const IE::point& end)
{
	return PointDistance(start, end) * kMovementCost;
}



static inline IE::point
HalfPoint(const IE::point& start, const IE::point& end)
{
	IE::point halfPoint;
	halfPoint.x = start.x + (end.x - start.x);
	halfPoint.y = start.y + (end.y - start.y);
	return halfPoint;
}


// Path
Path::Path()
	:
	fPoints(new PointList),
	fIterator(fPoints->begin())
{
}


Path::Path(const IE::point start, const IE::point end, test_function func)
	:
	fPoints(new PointList),
	fIterator(fPoints->begin())
{
	Set(start, end, func);
}


Path::~Path()
{
	delete fPoints;
}


void
Path::Set(const IE::point& start, const IE::point& end, test_function func)
{
	fPoints->erase(fPoints->begin(), fPoints->end());

	PathFinder pathFinder(2, func, true);
	PointList path = pathFinder.GeneratePath(start, end);

	for (PointList::const_iterator i = path.begin(); i != path.end(); i++)
		fPoints->push_back(*i);

	fIterator = fPoints->begin();
}


void
Path::Clear()
{
	fPoints->erase(fPoints->begin(), fPoints->end());
}


IE::point
Path::Start() const
{
	return *fPoints->begin();
}


IE::point
Path::End() const
{
	return *fPoints->rbegin();
}


void
Path::AddPoint(const IE::point& point, test_function func)
{
	PathFinder pathFinder(2, func, true);
	PointList path = pathFinder.GeneratePath(fPoints->front(), point);
	for (PointList::const_iterator i = path.begin(); i != path.end(); i++)
		fPoints->push_back(*i);
}


IE::point
Path::NextStep(const int& step)
{
	for (int i = 0; i < step; i++) {
		if (fIterator != fPoints->end())
			fIterator++;
	}
	if (fIterator == fPoints->end())
		return *fPoints->rbegin();
	return *fIterator;
}


bool
Path::IsEmpty() const
{
	return fPoints->empty();
}


bool
Path::IsEnd() const
{
	 return fIterator == fPoints->end();
}


typedef std::vector<point_node*> NodeVector;


// PathFinder
PathFinder::PathFinder(int16 step, test_function testFunc, bool checkNeighbors)
	:
	fStep(step),
	fTestFunction(testFunc),
	fCheckNeighbors(checkNeighbors),
	fDebugFunction(NULL)
{
}


PathFinder::~PathFinder()
{
}


bool
PathFinder::GenerateNodes(Bitmap* searchMap)
{
	// TODO: Not doing anything useful yet
	NodeVector nodes;
	for (int16 x = 0; searchMap->Width(); x++) {
		for (int16 y = 0; searchMap->Height(); y++) {
			IE::point point = {x, y};
			nodes.push_back(new point_node(point, NULL, 0));
		}
	}
	return true;
}


bool
PathFinder::IsEmpty() const
{
	return fPoints.empty();
}


/* static */
bool
PathFinder::IsInLineOfSight(const IE::point& start, const IE::point& end)
{
	PathFinder testPath(1);

	if (!testPath._IsPassable(start) || !testPath._IsPassable(end))
		return false;

	return testPath.CreateLineOfSightPath(start, end);
}


PointList
PathFinder::GeneratePath(const IE::point& start, const IE::point& end)
{
	//if (!_IsPassable(end))
	//	return false;

	/*if (Distance(start, end) > 40) {
		IE::point shorterEnd = HalfPoint(start, end);
		PathFinder shorterPath(2);
		PointList path = shorterPath.GeneratePath(start, shorterEnd);

	}*/
	IE::point maxReachableDirectly = start;

	// Try a line of sight path
	/*PathFinder lofPath(fStep, fTestFunction);
	lofPath.CreateLineOfSightPath(start, end);
	if (!lofPath.IsEmpty())
		maxReachableDirectly = *lofPath.fPoints.rbegin();
*/
	//if (IsCloseEnough(maxReachableDirectly, end))
	//	return true;

	ClosedNodes closedNodeList;

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	currentNode->cost_to_goal = Distance(currentNode->point, end)
		+ currentNode->cost;
	currentNode->open = true;
	closedNodeList.nodelist->push_back(currentNode);

	uint32 tries = PATHFIND_MAX_TRIES;
	bool found = false;
	const IE::point directions[] = {
		{ int16(-fStep), int16(-fStep) },
		{ int16(-fStep), 0 },
		{ int16(-fStep), int16(+fStep) },
		{ 0, int16(-fStep) },
		{ 0, int16(+fStep) },
		{ int16(+fStep), int16(-fStep) },
		{ int16(+fStep), 0 },
		{ int16(+fStep), int16(+fStep) }
	};
	while ((currentNode = closedNodeList.GetCheapestNode()) != NULL) {
		if (PointDistance(currentNode->point, end) < uint32(fStep)) {
			found = true;
			break;
		}

		if (--tries == 0)
			break;

		currentNode->open = false;

		// Add neighbours
		const size_t arraySize = sizeof(directions) / sizeof(directions[0]);
		for (size_t c = 0; c < arraySize; c++) {
			_AddIfPassable(currentNode->point + directions[c], *currentNode,
						   end, closedNodeList.nodelist);
		}
		if (fDebugFunction != NULL)
			fDebugFunction(currentNode->point);
	}

	if (!found)
		throw PathNotFoundException();

	point_node* last = closedNodeList.nodelist->back();
	_ReconstructPath(last);
	
	// remove the "current" position, it's useless
	fPoints.erase(fPoints.begin());
	
	return fPoints;
}


/* static */
IE::point
PathFinder::HalfPoint(const IE::point& start, const IE::point& end)
{
	IE::point half;
	half.x = start.x + end.x / 2;
	half.y = start.y + end.y / 2;
	return half;
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



uint32
PathFinder::MovementCost(const IE::point& pointA, const IE::point& pointB) const
{
	// Movement cost. Bigger when moving diagonally
	return (std::abs(pointA.x - pointB.x) < fStep)
			|| (std::abs(pointA.y - pointB.y) < fStep) ?
				 kMovementCost : kDiagMovementCost;
}


void
PathFinder::SetDebug(debug_function callback)
{
	fDebugFunction = callback;
}


bool
PathFinder::CreateLineOfSightPath(const IE::point& start, const IE::point& end)
{
	//assert(fClosedNodeList == NULL);
/*
	fPoints = new PointList;
	fClosedNodeList = new NodeList();

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
				return false;
			fPoints->push_back(point);
			cycle += shDelta;
			if (cycle > lgDelta) {
				cycle -= lgDelta;
				point.y += shStep;
			}
			point.x += lgStep;
		}
		if (!_IsPassable(point))
			return false;

		fPoints->push_back(point);
	}
	cycle = shDelta >> 1;
	while (std::abs(point.y - end.y) > fStep) {
		if (!_IsPassable(point))
			return false;
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
*/
	return true;
}


bool
PathFinder::_IsPassable(const IE::point& point) const
{
	return fTestFunction(point);
}


bool
PathFinder::_IsReachable(const IE::point& current, const IE::point& point) const
{
	if (!fCheckNeighbors)
		return _IsPassable(point);

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
}


void
PathFinder::_AddIfPassable(const IE::point& point,
		const point_node& current,
		const IE::point& goal,
		NodeList* nodeList)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list. If so, update it.
	NodeList::const_iterator i =
			std::find_if(nodeList->begin(), nodeList->end(),
							FindPoint(point));
	if (i != nodeList->end()) {
		_UpdateNodeCost(*i, current, goal);
		return;
	}

	// Otherwise, add it to the open list
	point_node* node = new point_node(point, &current, UINT_MAX);
	node->open = true;
	nodeList->push_back(node);
	_UpdateNodeCost(node, current, goal);
}


void
PathFinder::_AddNeighbors(const point_node& node,
		const IE::point& goal)
{

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


void
PathFinder::_ReconstructPath(point_node* goal)
{
	point_node* walkNode = goal;
	while (walkNode != NULL) {
		fPoints.push_front(walkNode->point);
		walkNode = const_cast<point_node*>(walkNode->parent);
	}
}


// ClosedNodes
ClosedNodes::ClosedNodes()
	:
	nodelist(nullptr)
{
	nodelist = new NodeList;
}


ClosedNodes::~ClosedNodes()
{
	NodeList::iterator i;
	for (i = nodelist->begin(); i != nodelist->end(); i++) {
		delete (*i);
	}
	delete nodelist;
	nodelist = NULL;
}


point_node*
ClosedNodes::GetCheapestNode() const
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (NodeList::const_iterator i = nodelist->begin();
			i != nodelist->end(); i++) {
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


// PathNotFoundException
PathNotFoundException::PathNotFoundException()
	:
	std::runtime_error("Path not found")
{
}

