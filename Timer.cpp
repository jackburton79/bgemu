/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#include "Timer.h"

#include <sys/time.h>

Timer::timer_map Timer::sTimers;

Timer::Timer(uint32 expirationTime)
	:
	fExpiration(expirationTime)
{
}


Timer::~Timer()
{
}


void
Timer::SetExpiration(uint32 expiration)
{
	fExpiration = expiration;
}


bool
Timer::Expired() const
{
	// TODO: These timers are relative to the game time,
	// reimplement correctly.
	struct timeval now;
	gettimeofday(&now, 0);
	if ((uint32)now.tv_usec >= fExpiration)
		return true;
	return false;
}


/* static */
void
Timer::Add(const char* name, uint32 expirationTime)
{
	sTimers[name] = new Timer(expirationTime);
}


/* static */
void
Timer::Remove(const char* name)
{
	Timer* timer = Get(name);
	if (timer != NULL) {
		sTimers.erase(name);
		delete timer;
	}
}


/* static */
Timer*
Timer::Get(const char* name)
{
	std::map<std::string, Timer*>::const_iterator i = sTimers.find(name);
	if (i == sTimers.end())
		return NULL;

	return i->second;
}
