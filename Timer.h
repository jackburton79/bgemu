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
	Timer(uint32 expirationTime);
	~Timer();

	void SetExpiration(uint32 timer);
	bool Expired() const;

	static void Add(const char* name, uint32 expirationTime = -1);
	static void Remove(const char* name);
	static Timer* Get(const char* string);

private:
	uint32 fExpiration;

	typedef std::map<std::string, Timer*> timer_map;
	static timer_map sTimers;
};

#endif /* __TIMER_H */
