/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */


#include "Timer.h"

#include "ResManager.h"

#include <sys/time.h>

#include <SDL.h>

Timer::timer_map Timer::sTimers;
uint32 Timer::sGameTime;

Timer::Timer(uint32 expirationTime)
	:
	fExpiration(expirationTime)
{
}

/*
Timer::~Timer()
{
}
*/

void
Timer::SetExpiration(uint32 expiration)
{
	fExpiration = sGameTime + expiration;
}


bool
Timer::Expired() const
{
	return sGameTime >= fExpiration;
}


/* static */
void
Timer::Wait(uint32 delay)
{
	SDL_Delay(delay);	
}


/* static */
void
Timer::AddOneShotTimer(uint32 interval, timer_func func, void* parameter)
{
	SDL_AddTimer(interval, func, parameter);	
}


/* static */
uint32
Timer::Ticks()
{
	return SDL_GetTicks();	
}


/* static */
void
Timer::Add(const char* name, uint32 expirationTime)
{
	std::string expiration = IDTable::GameTimeAt(expirationTime);
	std::cout << "Added timer " << name << " which expires in ";
	std::cout << expiration << "(" << std::dec << expirationTime;
	std::cout << ")" << std::endl;
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


/* static */
uint32
Timer::GameTime()
{
	return sGameTime;
}


/* static */
uint32
Timer::Hour()
{
	return 1;
}


/* static */
void
Timer::UpdateGameTime()
{
	// TODO: This way the time runs too fast
	sGameTime++;
}
