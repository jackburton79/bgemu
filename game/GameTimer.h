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

// Per IESDP (docs/iesdp-gh-pages/appendices/timers.htm), the IE tracks
// two separate clocks: CINGAME (in-game time - only advances via
// explicit time-skips like resting, not continuously) and CREAL (real
// time actually spent playing). SETGLOBALTIMER/GlobalTimerExpired use
// CINGAME; REALSETGLOBALTIMER/RealGlobalTimerExpired use CREAL. A
// GameTimer's type selects which clock it's measured against.
enum timer_type {
	TIMER_GLOBAL = 0,
	TIMER_REAL = 1
};

class GameTimer {
public:
	uint32 Get() const;

	static void DisposeTimers();

	void SetExpiration(uint32 timer);
	bool Expired() const;

	static void Add(const char* name, uint32 expirationTime = -1,
		timer_type type = TIMER_GLOBAL);
	static void Remove(const char* name);
	static GameTimer* Get(const char* string);
	static uint32 GameTime();

	// CREAL, in seconds - derived directly from Timer::Ticks()
	// (SDL_GetTicks(), already real wall-clock milliseconds since the
	// library initialized near game start), so unlike CINGAME/sGameTime
	// it doesn't need a per-tick increment and isn't affected by how
	// often Core::UpdateLogic() happens to run.
	static uint32 RealTime();

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
	GameTimer(uint32 expirationTime, timer_type type);
	//~GameTimer();

	uint32 fExpiration;
	timer_type fType;

	typedef std::map<std::string, GameTimer*> timer_map;
	static timer_map sTimers;
	static uint32 sGameTime;
};

