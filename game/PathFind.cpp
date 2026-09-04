#include "PathFind.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <vector>


#include "Bitmap.h"

static constexpr uint32 kMaxTries = 500000;

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


struct SearchNode {
	SearchNode(IE::point p, const SearchNode* parentNode, int nodeCost)
		:
		point(p),
		parent(parentNode),
		gCost(nodeCost),
		fCost(UINT_MAX),
		open(false)
	{
	};

	const IE::point point;
	const struct SearchNode* parent;
	uint32 gCost;
	uint32 fCost;
	bool open;
};


struct NodeCompare {
	bool operator()(const SearchNode* a, const SearchNode* b) const
	{
		return a->fCost > b->fCost;
	}
};

typedef std::unordered_map<IE::point, SearchNode*, PointHash, PointEqual> NodeMap;
typedef std::priority_queue<SearchNode*, std::vector<SearchNode*>, NodeCompare> OpenQueue;
typedef std::deque<SearchNode*> NodeList;

class NodeSearchContext {
public:
	NodeSearchContext();
	~NodeSearchContext();

	SearchNode* GetCheapestNode();
	void AddOpenNode(SearchNode* node);

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
	fIterator(fPoints.begin())
{
}


Path::Path(const IE::point start, const IE::point end, test_function func)
	:
	fIterator(fPoints.begin())
{
	Set(start, end, func);
}


Path::~Path()
{
}


void
Path::Set(const IE::point& start, const IE::point& end, test_function func)
{
	// Use 2 here so it's faster, we'll interpolate the path later
	PathFinder pathFinder(2, func, true);
	// This can throw an exception
	PointList path = pathFinder.GeneratePath(start, end);

	fPoints.swap(path);

	fIterator = fPoints.begin();

	fStats = pathFinder.Statistics();
}


void
Path::Clear()
{
	fPoints.clear();
	fIterator = fPoints.begin();
}


IE::point
Path::Start() const
{
	assert(!fPoints.empty());
	return *fPoints.begin();
}


IE::point
Path::End() const
{
	assert(!fPoints.empty());
	return *fPoints.rbegin();
}


void
Path::AddPoint(const IE::point& point, test_function func)
{
	PathFinder pathFinder(2, func, true);
	PointList path = pathFinder.GeneratePath(fPoints.front(), point);
	for (PointList::const_iterator i = path.begin(); i != path.end(); i++)
		fPoints.push_back(*i);
	fStats = pathFinder.Statistics();
}


IE::point
Path::NextStep(int step)
{
	if (fPoints.empty())
		return IE::point{ 0, 0 };

	while (step > 0 && fIterator != fPoints.end()) {
		++fIterator;
		--step;
	}

	if (fIterator == fPoints.end())
		return fPoints.back();

	return *fIterator;
}


bool
Path::IsEmpty() const
{
	return fPoints.empty();
}


bool
Path::IsEnd() const
{
	return fIterator == fPoints.end();
}


debug_function PathFinder::sDebugFunction;

// PathFinder
PathFinder::PathFinder(int16 step, test_function testFunc, bool checkNeighbors)
	:
	fStep(step),
	fTestFunction(testFunc),
	fCheckNeighbors(checkNeighbors)
{
	//std::cout << "PathFinder: step " << fStep << std::endl;
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
	SearchNode* currentNode = _InitializeSearch(searchContext, start, end);

	uint32 tries = kMaxTries;
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

	PointList pathPoints = _BuildPath(currentNode);
	_GetSmoothenPath(pathPoints);

	uint32 length = 0;
	for (auto it = std::next(pathPoints.begin()); it != pathPoints.end(); ++it) {
		auto prev = std::prev(it);
		length += Distance(*prev, *it);
	}

	fStats.path_nodes = pathPoints.size();
	fStats.path_length = length;

	return pathPoints;
}



bool
PathFinder::HasLineOfSight(const IE::point& from, const IE::point& to) const
{
	return _WalkLine(from, to, NULL);
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


SearchNode*
PathFinder::_InitializeSearch(NodeSearchContext& context, IE::point start, IE::point end)
{
	SearchNode* startNode = new SearchNode(start, NULL, 0);
	startNode->fCost = PointDistance(startNode->point, end) + startNode->gCost;
	startNode->open = true;
	context.nodelist.push_back(startNode);
	context.AddOpenNode(startNode);
	context.nodeMap[startNode->point] = startNode;

	fStats.generated_nodes++;

	return startNode;
}


PointList
PathFinder::_BuildPath(SearchNode* goalNode)
{
	PointList tmpPoints;
	SearchNode* last = goalNode;
	const SearchNode* walkNode = last;
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

	return pathPoints;
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
		const SearchNode& current,
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
	SearchNode* node = new SearchNode(point, &current, UINT_MAX);
	fStats.generated_nodes++;
	node->open = true;
	nodes->nodelist.push_back(node);
	nodes->nodeMap.emplace(node->point, node);
	_UpdateNodeCost(node, current, goal, *nodes);
}


void
PathFinder::_UpdateNodeCost(SearchNode* node, const SearchNode& current, const IE::point& goal,
							NodeSearchContext& closedNodeList) const
{
	const uint32 newCost = MovementCost(current.point,
			node->point) + current.gCost;
	if (newCost < node->gCost) {
		fStats.updated_nodes++;
		node->parent = &current;
		node->gCost = newCost;
		node->fCost = Distance(node->point, goal) + node->gCost;
		closedNodeList.AddOpenNode(node);
	}
}


bool
PathFinder::_WalkLine(const IE::point& from, const IE::point& to, PointList* outPoints) const
{
	int x0 = from.x;
	int y0 = from.y;

	int x1 = to.x;
	int y1 = to.y;

	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);

	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;

	int err = dx - dy;

	while (true) {
		IE::point p = { static_cast<int16>(x0), static_cast<int16>(y0) };

		if (!_IsPassable(p))
			return false;

		if (outPoints != NULL)
			outPoints->push_back(p);

		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;

		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}

		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}

	return true;
}


void
PathFinder::_GetSmoothenPath(PointList& path)
{
	if (path.size() < 3)
		return;

	std::vector<IE::point> original(path.begin(), path.end());
	std::vector<IE::point> result;
	result.reserve(original.size());
	result.push_back(original.front());

	size_t anchor = 0;
	while (anchor < original.size() - 1) {
		size_t farthest = anchor + 1;
		PointList line;

		// Find the farthest point reachable from 'anchor' in a straight,
		// fully passable line. Start from the far end so we grab the
		// longest straight run first.
		for (size_t candidate = original.size() - 1; candidate > anchor + 1; candidate--) {
			PointList tmp;
			if (_WalkLine(original[anchor], original[candidate], &tmp)) {
				farthest = candidate;
				line.swap(tmp);
				break;
			}
		}

		if (farthest == anchor + 1) {
			// No useful shortcut: keep the next point as-is.
			result.push_back(original[anchor + 1]);
			anchor++;
			continue;
		}

		// Replace the intermediate points with the same number of points,
		// resampled along the verified straight line (so they stay passable).
		std::vector<IE::point> lineVec(line.begin(), line.end());
		size_t neededInterior = farthest - anchor - 1;
		size_t lastIdx = lineVec.size() - 1;

		for (size_t k = 1; k <= neededInterior; k++) {
			double t = (double)k / (double)(neededInterior + 1);
			size_t idx = (size_t)std::lround(t * (double)lastIdx);
			if (lastIdx > 1)
				idx = std::max<size_t>(1, std::min(idx, lastIdx - 1));
			result.push_back(lineVec[idx]);
		}

		result.push_back(original[farthest]);
		anchor = farthest;
	}

	path.assign(result.begin(), result.end());
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


SearchNode*
NodeSearchContext::GetCheapestNode()
{
	while (!fOpenQueue.empty()) {
		SearchNode* node = fOpenQueue.top();

		fOpenQueue.pop();

		if (node->open)
			return node;
	}

	return nullptr;
}


void
NodeSearchContext::AddOpenNode(SearchNode* node)
{
	fOpenQueue.push(node);
}


// PathNotFoundException
PathNotFoundException::PathNotFoundException()
	:
	std::runtime_error("Path not found")
{
}

