/*
 * OutputConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "OutputConsole.h"

OutputConsole::OutputConsole(const GFX::rect& rect)
	:
	Console(rect),
	fQuit(false)
{
	_EnableOutputRedirect();
	fLock = SDL_CreateMutex();
	fThread = SDL_CreateThread(_UpdateFunction, this);
}


OutputConsole::~OutputConsole()
{
	fQuit = true;
	SDL_DestroyMutex(fLock);
	SDL_WaitThread(fThread, NULL);

	_DisableOutputRedirect();
}


void
OutputConsole::_EnableOutputRedirect()
{
	fOldBuf = std::cout.rdbuf();
	std::cout.rdbuf(fOutputBuffer.rdbuf());
}


void
OutputConsole::_DisableOutputRedirect()
{
	std::cout.rdbuf(fOldBuf);
}


void
OutputConsole::Update()
{
	// Write stdout to console
	Puts(fOutputBuffer.str().c_str());
	fOutputBuffer.str("");
}


/* static */
int
OutputConsole::_UpdateFunction(void *arg)
{
	OutputConsole* console = reinterpret_cast<OutputConsole*>(arg);
	while (!console->fQuit) {
		SDL_mutexP(console->fLock);
		console->Update();
		SDL_Delay(100);
		SDL_mutexV(console->fLock);
	}

	return 0;
}
