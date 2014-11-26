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
#include <string>

enum timer_type {
	TIMER_GLOBAL = 0
};


class Timer {
public:
	void SetExpiration(uint32 timer);
	bool Expired() const;

	typedef uint32 (*timer_func)(uint32 interval, void* parameter);
	
	static void AddOneShotTimer(uint32 time, timer_func function, void* parameter);
	
	static void Add(const char* name, uint32 expirationTime = -1);
	static void Remove(const char* name);
	static Timer* Get(const char* string);
	static uint32 GameTime();

	static uint32 Hour();

	static void UpdateGameTime();

private:
	Timer(uint32 expirationTime);
	//~Timer();

	uint32 fExpiration;

	typedef std::map<std::string, Timer*> timer_map;
	static timer_map sTimers;
	static uint32 sGameTime;
};


#endif /* __TIMER_H */
