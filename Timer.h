/*
 * Timer.h
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#ifndef __TIMER_H
#define __TIMER_H

#include "SupportDefs.h"

#include <map>

class Timer {
public:
	Timer();
	~Timer();

	void SetExpiration(uint32 timer);
	bool Expired() const;

	static void Add(int32 id);
	static void Remove(int32 id);
	static Timer* Get(int32 id);

private:
	uint32 fExpiration;

	typedef std::map<int32, Timer*> timer_map;
	static timer_map sTimers;
};

#endif /* __TIMER_H */
