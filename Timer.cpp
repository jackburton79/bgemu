/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#include "Timer.h"

#include <sys/time.h>

Timer::Timer()
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
