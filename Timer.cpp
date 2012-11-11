/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#include "Timer.h"

#include <sys/time.h>

std::map<int32, Timer*> Timer::sTimers;

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
Timer::Add(int32 id)
{
	sTimers[id] = new Timer();
}


/* static */
void
Timer::Remove(int32 id)
{
	delete sTimers[id];
	sTimers[id] = NULL;
}


/* static */
Timer*
Timer::Get(int32 id)
{
	return sTimers[id];
}
