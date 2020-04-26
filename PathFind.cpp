#include "PathFind.h"
#include "IETypes.h"
#include "RectUtils.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
//#include <queue>
#include <limits.h>

#define PATHFIND_MAX_TRIES 999000


struct FindPoint {
	FindPoint(const IE::point& point)
		: toFind(point) {};

	bool operator() (const IE::point& point) {
		return point == toFind;
	};
	const IE::point& toFind;
};


struct PointComparator {
	bool operator()(const IE::point& A, const IE::point& B) {		
		return std::tie(A.x, A.y) < std::tie(B.x, B.y);
	};
};



std::map<IE::point, IE::point, PointComparator> gPath;
std::map<IE::point, uint32, PointComparator> gCostsToGoal;
std::map<IE::point, uint32, PointComparator> gCostsToNode;


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


IE::point
PathFinder::_GeneratePath(const IE::point& start, const IE::point& goal)
{
	fPoints.clear();
	fGoal = goal;

	PointList openList;
	IE::point currentNode = start;
	gCostsToNode[currentNode] = 0;
	gCostsToGoal[currentNode] = Distance(currentNode, goal);
	openList.push_back(currentNode);

	uint32 tries = PATHFIND_MAX_TRIES;
	bool notFound = false;
	while (!openList.empty()) {
		currentNode = _GetCheapestNode(openList, goal);
		if (--tries == 0) {
			notFound = true;
			break;
		}

		if (PointSufficientlyClose(currentNode, goal))
			break;

		struct FindPoint pointFinder(currentNode);
		openList.remove_if(pointFinder);
		_AddNeighbors(currentNode, openList);

		if (fDebugFunction != NULL)
			fDebugFunction(currentNode);
	}

	if (notFound) {
		// TODO: Destination is unreachable.
		// Try to find a reachable point near destination
		return start;
	}

	_ReconstructPath(openList);
	
	// remove the "current" position, it's useless
	fPoints.erase(fPoints.begin());
	
	return fPoints.back();
}


void
PathFinder::_ReconstructPath(PointList& openList)
{
	std::list<IE::point>::reverse_iterator r = openList.rbegin();
	IE::point walkNode = *r;
	for (;;) {
		fPoints.push_front(walkNode);
		std::map<IE::point, IE::point>::iterator p;
		p = gPath.find(walkNode);		
		if (p == gPath.end())
			break;
		walkNode = (*p).second;
	}
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
	if (point.x < 0 || point.y < 0 || !_IsPassable(point))
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
PathFinder::_AddNeighbors(const IE::point& current, PointList& list)
{
	for (int x = -fStep; x <= fStep; x+= fStep) {
		for (int y = -fStep; y <= fStep; y+= fStep) {
			if (x == 0 && y == 0)
				continue;		
			IE::point neighbor = offset_point(current, x, y);
			if (!_IsReachable(current, neighbor))
				continue;
			std::map<IE::point, uint32>::iterator i;
			i = gCostsToGoal.find(neighbor);
			if (i == gCostsToGoal.end())
				gCostsToGoal[neighbor] = UINT_MAX;				
			i = gCostsToNode.find(neighbor);
			if (i == gCostsToNode.end())
				gCostsToNode[neighbor] = UINT_MAX;				
			uint32 newCost = gCostsToNode[current] +
					MovementCost(current, neighbor);
			if (newCost < gCostsToNode[neighbor]) {
				gPath[neighbor] = current;
				gCostsToNode[neighbor] = newCost;
				gCostsToGoal[neighbor] = gCostsToNode[neighbor]	+ Distance(neighbor, fGoal);
				PointList::iterator p = std::find_if(list.begin(), list.end(), FindPoint(neighbor));			
				if (p == list.end()) {
					list.push_back(neighbor);
				}
			}	
		}	
	}
}


IE::point
PathFinder::_GetCheapestNode(PointList& list, const IE::point& end)
{
	uint32 minCost = UINT_MAX;
	IE::point result;
	for (PointList::const_iterator i = list.begin();
			i != list.end(); i++) {
		IE::point node = *i;
		const uint32 totalCost = gCostsToGoal[node];
		if (totalCost < minCost) {
			result = node;
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
