/*
 * Timer.cpp
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */


#include "Timer.h"

#include "Log.h"
#include "ResManager.h"

#include <sstream>
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
	if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0)
		return false;
	return true;
}


/* static */
Timer*
Timer::Set(const char* name, uint32 delay)
{
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
	return Ticks() >= fExpirationTime;
}


void
Timer::Rearm()
{
	fExpirationTime = Ticks() + fDelay;
}


static uint32
oneshot_timer_callback(uint32 interval, void* castToFunctor)
{
	Timer::Functor* functor = reinterpret_cast<Timer::Functor*>(castToFunctor);
	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = (void*)functor->Function();
	userevent.data2 = functor->Parameter();

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);

	delete functor;

	return 0;
}


static uint32
periodic_timer_callback(uint32 interval, void* castToFunctor)
{
	Timer::Functor* functor = reinterpret_cast<Timer::Functor*>(castToFunctor);

	SDL_Event event;
	SDL_UserEvent userevent;

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = (void*)functor->Function();
	userevent.data2 = functor->Parameter();

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);

	// TODO: we are leaking the functor here

	return interval;
}


/* static */
void
Timer::AddOneShotTimer(uint32 delay, timer_function func, void* parameter)
{
	Functor* functor = new Functor(func, parameter);
	SDL_AddTimer(delay, oneshot_timer_callback, (void*)functor);
}


/* static */
void
Timer::AddPeriodicTimer(uint32 interval, timer_function func, void* parameter)
{
	Functor* functor = new Functor(func, parameter);
	// TODO: we should register this timer and remove it when not needed anymore
	SDL_AddTimer(interval, periodic_timer_callback, (void*)functor);
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


/* static */
void
Timer::WaitSync(uint32 start, uint32 maxDelay)
{
	int32 diff = (start + maxDelay) - Ticks();
	if (diff > 0)
		Wait(diff);
	else
		std::cerr << Log::Red << "WaitSync: TOO SLOW!" << std::endl;
	std::cerr << Log::Normal;
}


// Functor
Timer::Functor::Functor(timer_function func, void* parameter)
	:
	fFunction(func),
	fParameter(parameter)
{
}


timer_function&
Timer::Functor::Function()
{
	return fFunction;
}


void*
Timer::Functor::Parameter()
{
	return fParameter;
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
	std::ostringstream timeString;
	timeString << std::dec;
	timeString << (HourOfDay()) << ":" << (Minutes() % 60) << ":" << (Seconds() % 60);

	return timeString.str();
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
	if (sGameTime % 60 == 0)
		PrintTime();
	sGameTime++;
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
