/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */


#include "GameTimer.h"

#include "ResManager.h"

#include <sys/time.h>

#include <SDL.h>


// GameTimer
GameTimer::timer_map GameTimer::sTimers;
uint32 GameTimer::sGameTime;

GameTimer::GameTimer(uint32 expirationTime)
	:
	fExpiration(expirationTime)
{
}


uint32
GameTimer::Get() const
{
	return fExpiration;
}


/* static */
void
GameTimer::DisposeTimers()
{
	for (auto timer : sTimers) {
		delete timer.second;
	}
	sTimers.clear();
}


void
GameTimer::SetExpiration(uint32 expiration)
{
	fExpiration = sGameTime + expiration;
}


bool
GameTimer::Expired() const
{
	return sGameTime >= fExpiration;
}


/* static */
void
GameTimer::Add(const char* name, uint32 expirationTime)
{
	std::string expiration = IDTable::GameTimeAt(expirationTime);
	std::cout << "Added timer '" << name << "' which expires in ";
	std::cout << expiration << "(" << std::dec << expirationTime;
	std::cout << ")" << std::endl;
	timer_map::iterator i = sTimers.find(name);
	if (i != sTimers.end())
		i->second->SetExpiration(expirationTime);
	else
		sTimers[name] = new GameTimer(expirationTime);
}


/* static */
void
GameTimer::Remove(const char* name)
{
	GameTimer* timer = Get(name);
	if (timer != NULL) {
		sTimers.erase(name);
		delete timer;
	}
}


/* static */
GameTimer*
GameTimer::Get(const char* name)
{
	std::map<std::string, GameTimer*>::const_iterator i = sTimers.find(name);
	if (i == sTimers.end())
		return NULL;

	return i->second;
}


/* static */
uint32
GameTimer::GameTime()
{
	return sGameTime;
}


/* static */
uint32
GameTimer::Days()
{
	return Hours() / 24;
}


/* static */
uint32
GameTimer::Hours()
{
	return Minutes() / 60;
}


/* static */
uint32
GameTimer::Minutes()
{
	return Seconds() / 60;
}


/* static */
uint32
GameTimer::Seconds()
{
	return sGameTime;
}


/* static */
uint16
GameTimer::HourOfDay()
{
	return Hours() % 24;
}


/* static */
bool
GameTimer::IsDayTime()
{
	return (HourOfDay() > 6) && (HourOfDay() < 20);
}


/* static */
std::string
GameTimer::GameTimeString()
{
	// returns the in-game time as a string (in a 24 hour format)
	char timeString[64];
	::snprintf(timeString, sizeof(timeString), "%02u:%02u:%02u",
			   HourOfDay(), Minutes() % 60, Seconds() % 60);
	return timeString;
}


/* static */
void
GameTimer::PrintTime()
{
	std::cout << "GameTime: " << GameTimeString() << std::endl;
	std::cout << "ticks: " << Seconds() << std::endl;
}


/* static */
void
GameTimer::UpdateGameTime()
{
	//if (sGameTime % 60 == 0)
	//	PrintTime();
	sGameTime++;

	// TODO: Check timer expiration, add a trigger and delete
	// the timer
	/*std::map<std::string, GameTimer*>::const_iterator i;
	for (i = sTimers.begin(); i != sTimers.end(); i++) {

	}*/
}


/* static */
void
GameTimer::AdvanceTime(uint32 seconds)
{
	sGameTime += seconds;
}

/* static */
void
GameTimer::AdvanceTime(uint16 hours, uint16 minutes, uint16 seconds)
{
	sGameTime += hours * 60 * 60 + minutes * 60 + seconds;
}
