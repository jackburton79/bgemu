#include "PathFind.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <memory>

#include "Bitmap.h"
#include "Utils.h"

#define PATHFIND_MAX_TRIES 2000

const int kMovementCost = 1;
const int kDiagMovementCost = 2;

// Helper for hashing points for unordered_map
static inline uint64
HashPoint(const IE::point& p)
{
	return (static_cast<uint64>(p.x) << 32) | (static_cast<uint32>(p.y));
}

// Heuristic function (Manhattan distance)
static inline uint32
Heuristic(const IE::point& a, const IE::point& b)
{
	return static_cast<uint32>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

// Movement cost: straight or diagonal
static uint32
MovementCost(const IE::point& a, const IE::point& b, int fStep)
{
	return (std::abs(a.x - b.x) < fStep || std::abs(a.y - b.y) < fStep) ?
			kMovementCost : kDiagMovementCost;
}

struct AStarNode {
	IE::point point;
	uint32 gCost; // cost from start
	uint32 hCost; // heuristic to goal
	std::shared_ptr<AStarNode> parent;

	AStarNode(const IE::point& pt, uint32 g, uint32 h,
				std::shared_ptr<AStarNode> p = nullptr) :
			point(pt), gCost(g), hCost(h), parent(std::move(p))
	{
	}

	uint32
	fCost() const
	{
		return gCost + hCost;
	}
};

struct CompareNode {
	bool
	operator()(const std::shared_ptr<AStarNode>& a,
				const std::shared_ptr<AStarNode>& b) const
	{
		return a->fCost() > b->fCost();
	}
};

// ==== Path Implementation ====
Path::Path()
	:
	fPoints(new PointList), fIterator(fPoints->begin())
{
}

Path::Path(const IE::point start, const IE::point end, test_function func)
	:
	fPoints(nullptr), fIterator()
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
	fPoints = new PointList;
	PathFinder pathFinder(2, func, false);
	PointList path = pathFinder.GeneratePath(start, end);
	fPoints->insert(fPoints->end(), path.begin(), path.end());
	fIterator = fPoints->begin();
}


void
Path::Clear()
{
	assert(fPoints);
	fPoints->clear();
}


IE::point
Path::Start() const
{
	assert(fPoints && !fPoints->empty());
	return *fPoints->begin();
}


IE::point
Path::End() const
{
	assert(fPoints && !fPoints->empty());
	return *fPoints->rbegin();
}


void
Path::AddPoint(const IE::point& point, test_function func)
{
	assert(fPoints && !fPoints->empty());
	PathFinder pathFinder(2, func, true);
	PointList path = pathFinder.GeneratePath(fPoints->front(), point);
	fPoints->insert(fPoints->end(), path.begin(), path.end());
}


IE::point
Path::NextStep(const int& step)
{
	assert(fPoints);
	for (int i = 0; i < step && fIterator != fPoints->end(); ++i)
		++fIterator;
	if (fIterator == fPoints->end())
		return *fPoints->rbegin();
	return *fIterator;
}


bool
Path::IsEmpty() const
{
	assert(fPoints);
	return fPoints->empty();
}


bool
Path::IsEnd() const
{
	assert(fPoints);
	return fIterator == fPoints->end();
}


// ==== PathFinder Implementation ====
debug_function PathFinder::sDebugFunction;

PathFinder::PathFinder(int16 step, test_function testFunc, bool checkNeighbors)
	:
	fStep(step),
	fTestFunction(testFunc),
	fCheckNeighbors(checkNeighbors)
{
}


PathFinder::~PathFinder()
{
}


PointList
PathFinder::GeneratePath(const IE::point& start, const IE::point& end)
{
	if (!_IsPassable(end))
		throw PathNotFoundException();
	PointList pathPoints;

	std::priority_queue<std::shared_ptr<AStarNode>,
			std::vector<std::shared_ptr<AStarNode>>, CompareNode> openList;
	std::unordered_map<uint64, std::shared_ptr<AStarNode>> closedList;

	openList.push(std::make_shared<AStarNode>(start, 0, Heuristic(start, end)));
	const IE::point directions[] = {
			{ (int16)-fStep, (int16)-fStep },
			{ (int16)-fStep, 0 },
			{ (int16)-fStep, (int16)+fStep },
			{ 0, (int16)-fStep },
			{ 0, (int16)+fStep },
			{ (int16)+fStep, (int16)-fStep },
			{ (int16)+fStep, 0 },
			{ (int16)+fStep, (int16)+fStep }
		};

	uint32 tries = PATHFIND_MAX_TRIES;

	while (!openList.empty() && tries--) {
		auto current = openList.top();
		openList.pop();
		if (Heuristic(current->point, end) < uint32(fStep)) {
			// Found path
			PointList tmpPoints;
			for (auto node = current; node; node = node->parent)
				tmpPoints.push_front(node->point);
			tmpPoints.pop_front(); // remove start, optional
			pathPoints.insert(pathPoints.end(), tmpPoints.begin(),
								tmpPoints.end());
			//_GetSmoothenPath(pathPoints);
			return pathPoints;
		}
		closedList.emplace(HashPoint(current->point), current);

		for (const auto& dir : directions) {
			IE::point neighborPt = current->point + dir;
			if (neighborPt.x < 0 || neighborPt.y < 0
					|| !_IsReachable(current->point, neighborPt))
				continue;
			uint64 hash = HashPoint(neighborPt);
			if (closedList.count(hash))
				continue;

			uint32 tentative_g = current->gCost
					+ MovementCost(current->point, neighborPt, fStep);
			auto neighbor = std::make_shared<AStarNode>(
					neighborPt, tentative_g, Heuristic(neighborPt, end),
					current);

			openList.push(neighbor);
		}
		if (sDebugFunction)
			sDebugFunction(current->point);
	}
	throw PathNotFoundException();
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
	return (std::abs(point.x - goal.x) <= fStep)
			&& (std::abs(point.y - goal.y) <= fStep);
}

void
PathFinder::SetDebug(debug_function callback)
{
	sDebugFunction = callback;
}

bool
PathFinder::CreateLineOfSightPath(const IE::point& start, const IE::point& end)
{
	// Bresenham’s algorithm or similar can be used here for LoS, placeholder always returns true.
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
	int16 step = std::max(std::abs(point.x - current.x),
						std::abs(point.y - current.y));
	IE::point upperPt =
		{ current.x, int16(current.y - step) };
	IE::point leftPt =
		{ int16(current.x - step), current.y };
	IE::point bottomPt =
		{ current.x, int16(current.y + step) };
	IE::point rightPt =
		{ int16(current.x + step), current.y };

	if (point.x < current.x) {
		if (point.y < current.y) { // NW
			if (!_IsPassable(upperPt) && !_IsPassable(leftPt))
				return false;
		} else if (point.y > current.y) { // SW
			if (!_IsPassable(bottomPt) && !_IsPassable(leftPt))
				return false;
		}
	} else if (point.x > current.x) {
		if (point.y < current.y) { // NE
			if (!_IsPassable(upperPt) && !_IsPassable(rightPt))
				return false;
		} else if (point.y > current.y) { // SE
			if (!_IsPassable(bottomPt) && !_IsPassable(rightPt))
				return false;
		}
	}
	return true;
}

void
PathFinder::_GetSmoothenPath(PointList& pointList)
{
	for (auto p = pointList.begin(); p != pointList.end();) {
		IE::point pointA = *p;
		auto current = p++;
		if (p == pointList.end())
			break;
		IE::point pointB = *p;
		IE::point halfPoint = HalfPoint(pointA, pointB);
		pointList.insert(current, halfPoint);
	}
}

// ==== Exception ====
PathNotFoundException::PathNotFoundException() :
		std::runtime_error("Path not found")
{
}
