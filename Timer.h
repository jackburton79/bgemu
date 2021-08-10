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
	typedef uint32 (*timer_func)(uint32 interval, void* parameter);

	static bool Initialize();
	static Timer* Set(const char* name, uint32 delay);
	static Timer* Get(const char* name);

	bool Expired();
	void Rearm();

	static void Wait(uint32 delay);
	static void WaitSync(uint32 start, uint32 maxDelay);
	static void AddPeriodicTimer(uint32 time, timer_func function, void* parameter);
	static uint32 Ticks();

private:
	Timer(uint32 delay);

	uint32 fDelay;
	uint32 fExpirationTime;

	typedef std::map<std::string, Timer*> timer_map;
	static timer_map sTimers;
};


class GameTimer {
public:
	void SetExpiration(uint32 timer);
	bool Expired() const;
	
	static void Add(const char* name, uint32 expirationTime = -1);
	static void Remove(const char* name);
	static GameTimer* Get(const char* string);
	static uint32 GameTime();

	static uint32 Days();
	static uint32 Hours();
	static uint32 Minutes();
	static uint32 Seconds();
	static uint16 HourOfDay();
	static bool IsDayTime();

	static std::string GameTimeString();
	static void PrintTime();
	static void UpdateGameTime();
	static void AdvanceTime(uint32 seconds);
	static void AdvanceTime(uint16 hours, uint16 minutes, uint16 seconds);

private:
	GameTimer(uint32 expirationTime);
	//~GameTimer();

	uint32 fExpiration;

	typedef std::map<std::string, GameTimer*> timer_map;
	static timer_map sTimers;
	static uint32 sGameTime;
};


#endif /* __TIMER_H */
