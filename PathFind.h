#ifndef __PATHFIND_H
#define __PATHFIND_H

#include "IETypes.h"


typedef bool(*test_function)(const IE::point& start);
typedef void(*debug_function)(const IE::point& pt);


class PathFinderImpl;
class PathFinder {
public:
	const static int kStep = 5;

	PathFinder(int step = kStep, test_function func = IsPassableDefault);
	~PathFinder();

	IE::point SetPoints(const IE::point& start, const IE::point& end);
	IE::point NextWayPoint();
	bool IsEmpty() const;

	void SetDebug(debug_function callback);

	static bool IsPassableDefault(const IE::point& start) { return true; };
	static bool IsStraightlyReachable(const IE::point& start, const IE::point& end);

private:
	PathFinderImpl* fImplementation;
};


#endif // __PATHFIND_H
