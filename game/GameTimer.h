/*
 * GameTimer.h
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#pragma once
#include "SupportDefs.h"

#include <map>
#include <string>

#include "Timer.h"

enum timer_type {
	TIMER_GLOBAL = 0
};

class GameTimer {
public:
	uint32 Get() const;

	static void DisposeTimers();

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

