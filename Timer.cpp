/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#include "Timer.h"

#include <sys/time.h>

Timer::timer_map Timer::sTimers;

Timer::Timer()
	:
	fExpiration(-1)
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
	struct timeval now;
	gettimeofday(&now, 0);
	if ((uint32)now.tv_usec >= fExpiration)
		return true;
	return false;
}


/* static */
void
Timer::Add(const char* name)
{
	sTimers[name] = new Timer();
}


/* static */
void
Timer::Remove(const char* name)
{
	sTimers.erase(name);
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
