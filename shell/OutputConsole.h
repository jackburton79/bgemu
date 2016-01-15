/*
 * OutputConsole.h
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#ifndef OUTPUTCONSOLE_H_
#define OUTPUTCONSOLE_H_

#include "Console.h"
#include "SDL.h"

#include <iostream>

class OutputConsole: public Console {
public:
	OutputConsole(const GFX::rect& rect);
	~OutputConsole();

	bool HasOutputRedirected() const;
	void EnableRedirect();
	void DisableRedirect();

	void Update();

private:
	void _EnableOutputRedirect();
	void _DisableOutputRedirect();

	static int _UpdateFunction(void *arg);

	std::streambuf* fOldBuf;
	SDL_Thread* fThread;
	SDL_mutex* fLock;
	bool fOutputRedirected;
	bool fQuit;
};

#endif /* OUTPUTCONSOLE_H_ */
