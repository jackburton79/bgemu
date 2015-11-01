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

Timer::Timer(uint32 delay)
	:
	fDelay(delay),
	fExpirationTime(Ticks() + delay)
{
}


/* static */
bool
Timer::Initialize()
{
	SDL_InitSubSystem(SDL_INIT_TIMER);
	return true;
}


/* static */
Timer*
Timer::Set(const char* name, uint32 delay)
{
	std::cerr << "Set timer " << name << " which expires in ";
	std::cerr << std::dec << delay << " usecs" << std::endl;
	sTimers[name] = new Timer(delay);
	return sTimers[name];
}


/*static */
Timer*
Timer::Get(const char* name)
{
	timer_map::iterator i = sTimers.find(name);
	if (i == sTimers.end())
		return NULL;
	return i->second;
}


bool
Timer::Expired()
{
	std::cout << std::dec << "Expired ? " << Ticks() << ", " << fExpirationTime << std::endl;
	return Ticks() >= fExpirationTime;
}


void
Timer::Rearm()
{
	std::cout << "Timer rearmed" << std::endl;
	fExpirationTime = Ticks() + fDelay;
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
Timer::Wait(uint32 delay)
{
	SDL_Delay(delay);
}


// GameTimer
GameTimer::timer_map GameTimer::sTimers;
uint32 GameTimer::sGameTime;

GameTimer::GameTimer(uint32 expirationTime)
	:
	fExpiration(expirationTime)
{
}

/*
GameTimer::~GameTimer()
{
}
*/

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
	std::cout << "Added timer " << name << " which expires in ";
	std::cout << expiration << "(" << std::dec << expirationTime;
	std::cout << ")" << std::endl;
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
GameTimer::Hour()
{
	return 1;
}


/* static */
void
GameTimer::UpdateGameTime()
{
	// TODO: This way the time runs too fast
	sGameTime++;
}
