#include "PathFind.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>


#include "Bitmap.h"

#define PATHFIND_MAX_TRIES 9000000

const int kMovementCost = 1;
const int kDiagMovementCost = 2;


struct PointHash {
	std::size_t operator()(const IE::point& p) const
	{
		return (std::hash<int>()(p.x) << 16) ^ std::hash<int>()(p.y);
	}
};


struct PointEqual {
	bool operator()(const IE::point &a, const IE::point& b) const
	{
		return a.x == b.x && a.y == b.y;
	}
};


struct NodeCompare {
	bool operator()(const point_node* a, const point_node* b) const
	{
		return a->cost_to_goal > b->cost_to_goal;
	}
};

typedef std::unordered_map<IE::point, point_node*, PointHash, PointEqual> NodeMap;
typedef std::priority_queue<point_node*, std::vector<point_node*>, NodeCompare> OpenQueue;
typedef std::deque<point_node*> NodeList;

class NodeSearchContext {
public:
	NodeSearchContext();
	~NodeSearchContext();
	point_node* GetCheapestNode();
	void AddOpenNode(point_node* node);

	NodeList nodelist;
	NodeMap nodeMap;

private:
	OpenQueue fOpenQueue;
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


// PathFindStats
void
PathFindStats::Dump() const
{
	std::cout
		<< "Generated: " << generated_nodes << std::endl
		<< "Expanded : " << expanded_nodes << std::endl
		<< "Updated  : " << updated_nodes << std::endl
		<< "Path length : " << path_length << std::endl
		<< "Path points  : " << path_nodes <<  std::endl;
}


void
PathFindStats::Reset()
{
	generated_nodes = 0;
	expanded_nodes = 0;
	updated_nodes = 0;
	max_open_nodes = 0;

	path_nodes = 0;
	path_length = 0;
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
	delete fPoints;
	fPoints = nullptr;

	fPoints = new PointList;

	// Use 2 here so it's faster, we'll interpolate the path later
	PathFinder pathFinder(2, func, true);
	// This can throw an exception
	PointList path = pathFinder.GeneratePath(start, end);

	for (PointList::const_iterator i = path.begin(); i != path.end(); i++) {
		fPoints->push_back(*i);
	}

	fIterator = fPoints->begin();

	fStats = pathFinder.Statistics();
}


void
Path::Clear()
{
	assert(fPoints != NULL);
	fPoints->erase(fPoints->begin(), fPoints->end());
	fIterator = fPoints->begin();
}


IE::point
Path::Start() const
{
	assert(fPoints != NULL);
	assert(!fPoints->empty());
	return *fPoints->begin();
}


IE::point
Path::End() const
{
	assert(fPoints != NULL);
	assert(!fPoints->empty());
	return *fPoints->rbegin();
}


void
Path::AddPoint(const IE::point& point, test_function func)
{
	assert(fPoints != NULL);
	PathFinder pathFinder(2, func, true);
	PointList path = pathFinder.GeneratePath(fPoints->front(), point);
	for (PointList::const_iterator i = path.begin(); i != path.end(); i++)
		fPoints->push_back(*i);
	fStats = pathFinder.Statistics();
}


IE::point
Path::NextStep(const int& step)
{
	assert(fPoints != NULL);
	if (fPoints->empty())
		return IE::point{0, 0};

	for (int i = 0; i < step; i++) {
		if (fIterator != fPoints->end())
			fIterator++;
	}
	if (fIterator == fPoints->end())
		return fPoints->back();

	return *fIterator;
}


bool
Path::IsEmpty() const
{
	assert(fPoints != NULL);
	return fPoints->empty();
}


bool
Path::IsEnd() const
{
	assert(fPoints != NULL);
	return fIterator == fPoints->end();
}


debug_function PathFinder::sDebugFunction;

// PathFinder
PathFinder::PathFinder(int16 step, test_function testFunc, bool checkNeighbors)
	:
	fStep(step),
	fTestFunction(testFunc),
	fCheckNeighbors(checkNeighbors)
{
	std::cout << "PathFinder: step " << fStep << std::endl;
}


PathFinder::~PathFinder()
{
}


PointList
PathFinder::GeneratePath(const IE::point& start, const IE::point& end)
{
	fStats.Reset();

	if (!_IsPassable(end))
		throw PathNotFoundException();
#if 0
	std::cout << "GeneratePath(start: " << start.x << ", " << start.y << " -> ";
	std::cout << end.x << ", " << end.y << ")" << std::endl;
#endif

	NodeSearchContext searchContext;
	point_node* currentNode = _InitializeSearch(searchContext, start, end);

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

	const size_t arraySize = sizeof(directions) / sizeof(directions[0]);
	while ((currentNode = searchContext.GetCheapestNode()) != NULL) {
		fStats.expanded_nodes++;
		if (IsCloseEnough(currentNode->point, end)) {
			found = true;
			break;
		}

		if (--tries == 0)
			break;

		currentNode->open = false;

		// Add neighbours
		for (size_t c = 0; c < arraySize; c++) {
			_AddIfPassable(currentNode->point + directions[c], *currentNode,
						end, &searchContext);
		}
		if (sDebugFunction != NULL)
			sDebugFunction(currentNode->point);
	}

	if (!found)
		throw PathNotFoundException();

	PointList tmpPoints;
	point_node* last = currentNode;
	const point_node* walkNode = last;
	while (walkNode != NULL) {
		tmpPoints.push_front(walkNode->point);
		walkNode = walkNode->parent;
	}

	// remove the "current" position, it's useless
	if (!tmpPoints.empty())
		tmpPoints.erase(tmpPoints.begin());

	PointList pathPoints;
	PointList::iterator p;
	for (p = tmpPoints.begin(); p != tmpPoints.end(); p++) {
		pathPoints.push_back(*p);
	}

	//_GetSmoothenPath(pathPoints);

	uint32 length = 0;
	for (auto it = std::next(pathPoints.begin()); it != pathPoints.end(); ++it) {
		auto prev = std::prev(it);
		length += Distance(*prev, *it);
	}

	fStats.path_nodes = pathPoints.size();
	fStats.path_length = length;

	return pathPoints;
}


const PathFindStats&
PathFinder::Statistics() const
{
	return fStats;
}


IE::point
PathFinder::HalfPoint(const IE::point& start, const IE::point& end)
{
	IE::point half;
	half.x = start.x + (end.x - start.x) / 2;
	half.y = start.y + (end.y - start.y) / 2;
	half.x -= half.x % fStep;
	half.y -= half.y % fStep;
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
	sDebugFunction = callback;
}


point_node*
PathFinder::_InitializeSearch(NodeSearchContext& context, IE::point start, IE::point end)
{
	point_node* startNode = new point_node(start, NULL, 0);
	startNode->cost_to_goal = PointDistance(startNode->point, end) + startNode->cost;
	startNode->open = true;
	context.nodelist.push_back(startNode);
	context.AddOpenNode(startNode);
	context.nodeMap[startNode->point] = startNode;

	fStats.generated_nodes++;

	return startNode;
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
		NodeSearchContext* nodes)
{
	if (point.x < 0 || point.y < 0
			|| !_IsReachable(current.point, point))
		return;

	// Check if point is in closed list. If so, update it.
	auto it = nodes->nodeMap.find(point);
	if (it != nodes->nodeMap.end()) {
		_UpdateNodeCost(it->second, current, goal, *nodes);
		return;
	}

	// Otherwise, add it to the open list
	point_node* node = new point_node(point, &current, UINT_MAX);
	fStats.generated_nodes++;
	node->open = true;
	nodes->nodelist.push_back(node);
	nodes->nodeMap.emplace(node->point, node);
	_UpdateNodeCost(node, current, goal, *nodes);
}


void
PathFinder::_UpdateNodeCost(point_node* node, const point_node& current, const IE::point& goal,
							NodeSearchContext& closedNodeList) const
{
	const uint32 newCost = MovementCost(current.point,
			node->point) + current.cost;
	if (newCost < node->cost) {
		fStats.updated_nodes++;
		node->parent = &current;
		node->cost = newCost;
		node->cost_to_goal = Distance(node->point, goal) + node->cost;
		closedNodeList.AddOpenNode(node);
	}
}


void
PathFinder::_GetSmoothenPath(PointList& pointList)
{
	PointList::iterator p;
	for (p = pointList.begin(); p != pointList.end(); p++) {
		IE::point pointA = *p;
		PointList::iterator current = p;
		p++;
		if (p == pointList.end())
			break;
		IE::point pointB = *p;
		IE::point halfPoint = HalfPoint(pointA, pointB);
		pointList.insert(current, halfPoint);
	}
}


// NodeSearchContext
NodeSearchContext::NodeSearchContext()
{
}


NodeSearchContext::~NodeSearchContext()
{
	for (auto* node : nodelist)
		delete node;
}


point_node*
NodeSearchContext::GetCheapestNode()
{
	while (!fOpenQueue.empty()) {
		point_node* node = fOpenQueue.top();

		fOpenQueue.pop();

		if (node->open)
			return node;
	}

	return nullptr;
}


void
NodeSearchContext::AddOpenNode(point_node* node)
{
	fOpenQueue.push(node);
}


// PathNotFoundException
PathNotFoundException::PathNotFoundException()
	:
	std::runtime_error("Path not found")
{
}

