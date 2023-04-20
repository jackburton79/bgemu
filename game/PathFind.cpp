#include "PathFind.h"
#include "RectUtils.h"
#include "Utils.h"

#include <algorithm>
#include <assert.h>
#include <climits>
#include <cmath>
#include <deque>
#include <list>
#include <map>
#include <queue>


#include <memory>

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


typedef std::vector<point_node*> NodeVector;
typedef std::deque<point_node*> NodeList;
typedef std::deque<IE::point> PointList;


class PathFinderImpl {
public:
	PathFinderImpl(int16 step, test_function func, bool checkNeighbors);
	~PathFinderImpl();

	bool GeneratePath(const IE::point& start, const IE::point& end);
	void GetPoints(std::vector<IE::point>& points) const;
	PointList* Points();

	void SetDebug(debug_function callback);

private:
	int16 fStep;
	PointList* fPoints;
	NodeList* fClosedNodeList;
	test_function fTestFunction;
	bool fCheckNeighbors;

	debug_function fDebugFunction;

	bool IsCloseEnough(const IE::point& point, const IE::point& goal) const;
	uint32 MovementCost(const IE::point& pointA, const IE::point& pointB) const;

	bool _IsPassable(const IE::point& point) const;
	bool _IsReachable(const IE::point& current, const IE::point& point) const;
	void _AddIfPassable(const IE::point& point,
			const point_node& node,
			const IE::point& goal);
	void _AddNeighbors(const point_node& node,
			const IE::point& goal);
	void _UpdateNodeCost(point_node* node, const point_node& current,
			const IE::point& goal) const;
	point_node* _GetCheapestNode() const;
	void _ReconstructPath(point_node* goal);
};


PathFinder::PathFinder(int step, test_function testFunc, bool checkNeighbors)
	:
	fImplementation(NULL)
{
	fImplementation = new PathFinderImpl(1, testFunc, checkNeighbors);
}


PathFinder::~PathFinder()
{
	delete fImplementation;
}


IE::point
PathFinder::SetPoints(const IE::point& start, const IE::point& end)
{
	// TODO: Return a bool here
	if (!fImplementation->GeneratePath(start, end))
		throw std::runtime_error("PathFinder::SetPoints: cannot create path");
	return fImplementation->Points()->back();
}


void
PathFinder::GetPoints(std::vector<IE::point>& points) const
{
	fImplementation->GetPoints(points);
}


void
PathFinder::SetDebug(debug_function callback)
{
	fImplementation->SetDebug(callback);
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


IE::point
PathFinder::NextWayPoint(const int& step)
{
	IE::point point;
	for (int i = 0; i < step; i++) {
		if (fImplementation->Points()->empty())
			break;
		point = fImplementation->Points()->front();
		fImplementation->Points()->pop_front();
	}
	return point;
}


bool
PathFinder::IsEmpty() const
{
	return fImplementation->Points() == NULL || fImplementation->Points()->empty();
}


/* static */
bool
PathFinder::IsStraightlyReachable(const IE::point& start, const IE::point& end)
{
	PathFinder testPath(1);

	//if (!testPath._IsPassable(start) || !testPath._IsPassable(end))
		return false;

	//return testPath._CreateDirectPath(start, end) == end;
}


// PathFinderImpl
PathFinderImpl::PathFinderImpl(int16 step, test_function testFunc, bool checkNeighbors)
	:
	fStep(step),
	fPoints(NULL),
	fClosedNodeList(NULL),
	fTestFunction(testFunc),
	fCheckNeighbors(checkNeighbors),
	fDebugFunction(NULL)
{
}


PathFinderImpl::~PathFinderImpl()
{
	delete fPoints;
	delete fClosedNodeList;
}


static void
EmptyClosedList(NodeList*& pointList)
{
	NodeList::iterator i;
	for (i = pointList->begin(); i != pointList->end(); i++) {
		delete (*i);
	}
	delete pointList;
	pointList = NULL;
}


bool
PathFinderImpl::GeneratePath(const IE::point& start, const IE::point& end)
{
	delete fPoints;
	fPoints = NULL;

	if (!_IsPassable(end))
		return false;

	IE::point maxReachableDirectly = start;
	if (IsCloseEnough(maxReachableDirectly, end))
		return true;

	fPoints = new PointList;
	fClosedNodeList = new NodeList();

	point_node* currentNode = new point_node(maxReachableDirectly, NULL, 0);
	currentNode->cost_to_goal = Distance(currentNode->point, end)
		+ currentNode->cost;
	currentNode->open = true;
	fClosedNodeList->push_back(currentNode);

	uint32 tries = PATHFIND_MAX_TRIES;
	bool found = false;
	while ((currentNode = _GetCheapestNode()) != NULL) {
		// TODO: this could never happen if step is > 1
		if (currentNode->point == end) {
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
		delete fPoints;
		fPoints = NULL;
		return false;
	}

	point_node* last = fClosedNodeList->back();
	_ReconstructPath(last);
	EmptyClosedList(fClosedNodeList);
	
	// remove the "current" position, it's useless
	fPoints->erase(fPoints->begin());
	
	return true;
}


bool
PathFinderImpl::IsCloseEnough(const IE::point& point, const IE::point& goal) const
{
#if 0
	return pointA == pointB;
#else

	return (std::abs(point.x - goal.x) <= fStep)
		&& (std::abs(point.y - goal.y) <= fStep);
#endif
}


PointList*
PathFinderImpl::Points()
{
	return fPoints;
}


uint32
PathFinderImpl::MovementCost(const IE::point& pointA, const IE::point& pointB) const
{
	// Movement cost. Bigger when moving diagonally
	return (std::abs(pointA.x - pointB.x) < fStep)
			|| (std::abs(pointA.y - pointB.y) < fStep) ?
				 kMovementCost : kDiagMovementCost;
}


void
PathFinderImpl::GetPoints(std::vector<IE::point>& points) const
{
	for (PointList::iterator i = fPoints->begin(); i != fPoints->end(); i++)
		points.push_back(*i);
}


void
PathFinderImpl::SetDebug(debug_function callback)
{
	fDebugFunction = callback;
}


bool
PathFinderImpl::_IsPassable(const IE::point& point) const
{
	return fTestFunction(point);
}


bool
PathFinderImpl::_IsReachable(const IE::point& current, const IE::point& point) const
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
PathFinderImpl::_AddIfPassable(const IE::point& point,
		const point_node& current,
		const IE::point& goal)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list. If so, update it.
	NodeList::const_iterator i =
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
PathFinderImpl::_AddNeighbors(const point_node& node,
		const IE::point& goal)
{
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
	const size_t arraySize = sizeof(directions) / sizeof(directions[0]);
	for (size_t c = 0; c < arraySize; c++)
		_AddIfPassable(node.point + directions[c], node, goal);
}


void
PathFinderImpl::_UpdateNodeCost(point_node* node, const point_node& current, const IE::point& goal) const
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
PathFinderImpl::_GetCheapestNode() const
{
	uint32 minCost = UINT_MAX;
	point_node* result = NULL;
	for (NodeList::const_iterator i = fClosedNodeList->begin();
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
PathFinderImpl::_ReconstructPath(point_node* goal)
{
	point_node* walkNode = goal;
	while (walkNode != NULL) {
		fPoints->push_front(walkNode->point);
		walkNode = const_cast<point_node*>(walkNode->parent);
	}
}
