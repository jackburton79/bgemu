/*
 * GameConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "GameConsole.h"

#include "Commands.h"
#include "Log.h"
#include "ShellCommand.h"

#include <SDL.h>

#include "Core.h"

#include <fstream>
#include <iostream>
#include <string>

struct LockContext {
	LockContext()
		:
		SDLThread(nullptr),
		SDLLock(nullptr)
	{

	}

	~LockContext()
	{
		SDL_WaitThread(SDLThread, NULL);
		SDL_DestroyMutex(SDLLock);
	}

	SDL_Thread* SDLThread;
	SDL_mutex* SDLLock;
};


struct CommandSorter {
	bool operator()(ShellCommand* a, ShellCommand* b) const {
		return a->Command() < b->Command();
	}
};


GameConsole::GameConsole(const GFX::rect& rect, bool redirect)
	:
	Console(rect),
	fOldBuf(NULL),
	fLockContext(nullptr),
	fOutputRedirected(false),
	fQuit(false)
{
	if (redirect)
		_EnableOutputRedirect();
	fLockContext = new LockContext();

	fLockContext->SDLLock = SDL_CreateMutex();
	fLockContext->SDLThread = SDL_CreateThread(_UpdateFunction, "ConsoleThread", this);
}


GameConsole::~GameConsole()
{
	fQuit = true;

	delete fLockContext;

	_DisableOutputRedirect();

	std::list<ShellCommand*>::iterator command;
	for (command = fCommands.begin();
			command != fCommands.end(); command++) {
		delete *command;
	}
}


void
GameConsole::Initialize()
{
	AddCommands(this);
}


void
GameConsole::ShowHelp()
{
	std::list<ShellCommand*>::iterator i;
	CommandSorter sorter;
	fCommands.sort(sorter);
	for (i = fCommands.begin();
			i != fCommands.end(); i++) {
		ShellCommand* shellCommand = *i;
		std::cout << shellCommand->Command() << std::endl;
	}
}


void
GameConsole::AddCommand(ShellCommand* command)
{
	fCommands.push_back(command);
}


void
GameConsole::HandleInput(uint8 c)
{
	switch (c) {
		case SDLK_RETURN:
			_ExecuteCommand(fBuffer);
			fBuffer = "";
			break;
		case SDLK_BACKSPACE:
			if (fBuffer.length() > 0)
				fBuffer.resize(fBuffer.length() - 1);
			_ParseCharacter(c);
			break;
		case SDLK_TAB:
		{
			size_t pos = fBuffer.size();
			std::string fullCommand = _FindCompleteCommand(fBuffer);
			fBuffer = fullCommand;
			while (pos < fBuffer.length()) {
				_ParseCharacter(fBuffer[pos]);
				pos++;
			}
			break;
		}
		default:
			fBuffer.push_back(c);
			_ParseCharacter(c);
	}
}


void
GameConsole::ExecuteCommand(const std::string& line)
{
	_ExecuteCommand(line);
}


void
GameConsole::Update()
{
	// Write stdout to console
	Puts(fOutputBuffer.str().c_str());
	fOutputBuffer.str("");
}


/* static */
int
GameConsole::_UpdateFunction(void *arg)
{
	GameConsole* console = reinterpret_cast<GameConsole*>(arg);
	while (!console->fQuit) {
		if (SDL_LockMutex(console->fLockContext->SDLLock) == 0) {
			console->Update();
			SDL_Delay(50);
			SDL_UnlockMutex(console->fLockContext->SDLLock);
		}
	}

	return 0;
}


bool
GameConsole::HasOutputRedirected() const
{
	return fOutputRedirected;
}


void
GameConsole::EnableRedirect()
{
	_EnableOutputRedirect();
}


void
GameConsole::DisableRedirect()
{
	_DisableOutputRedirect();
}


void
GameConsole::_ExecuteCommand(const std::string& line)
{
	std::string command;
	std::istringstream cmdStream(line);
	cmdStream >> command;
	std::string args;
	try {
		size_t pos = cmdStream.tellg();
		cmdStream.seekg(pos + 1);
		try {
			args = cmdStream.str().substr(cmdStream.tellg());
		} catch (...) {
			// some commands are argless
		}
		std::cout << std::endl;
		ShellCommand* shellCommand = _FindCommand(command);
		if (shellCommand != NULL)
			(*shellCommand)(args.c_str());
		else {
			std::cout << "Invalid Command!" << std::endl;
			ShowHelp();
		}
	} catch (...) {
		std::cerr << Log::Red;
		std::cerr << "_ExecuteCommand(): " << line << " failed!" << std::endl;
		std::cerr << Log::Normal;
	}
}


void
GameConsole::_EnableOutputRedirect()
{
	std::cout << "GameConsole::EnablRedirect(): Check console for debug output from now on" << std::endl;
	fOldBuf = std::cout.rdbuf();
	std::cout.rdbuf(fOutputBuffer.rdbuf());
	fOutputRedirected = true;
}


void
GameConsole::_DisableOutputRedirect()
{
	if (fOutputRedirected) {
		std::cout.rdbuf(fOldBuf);
		fOutputRedirected = false;
		std::cout << "GameConsole::DisableRedirect(): Debug output on stdout/stderr" << std::endl;
	}
}


ShellCommand*
GameConsole::_FindCommand(const std::string& cmd)
{
	std::list<ShellCommand*>::iterator i;
	for (i = fCommands.begin();
			i != fCommands.end(); i++) {
		ShellCommand* shellCommand = *i;
		if (::strcasecmp(shellCommand->Command().c_str(), cmd.c_str()) == 0)
			return shellCommand;
	}

	return NULL;
}


std::string
GameConsole::_FindCompleteCommand(const std::string& partialCommand) const
{
	// TODO: Returns only the first matching command
	std::list<ShellCommand*>::const_iterator i;
	for (i = fCommands.begin();
			i != fCommands.end(); i++) {
		std::string fullCmd = (*i)->Command();
		if (::strncasecmp(fullCmd.c_str(), partialCommand.c_str(),
						partialCommand.length()) == 0) {
			return fullCmd;
		}
	}
	return partialCommand;
}


// Runs every non-blank, non-'#'-comment line of a script file as a console
// command, in order, printing each before running it (so the transcript
// is self-documenting) - console commands already print their own output
// to stdout (unredirected by default, see GameConsole's constructor
// comment), which is exactly what a headless ASan test run captures.
void
GameConsole::RunFile(const std::string& path)
{
	std::ifstream file(path.c_str());
	if (!file.is_open()) {
		std::cerr << "GameConsole::RunFile(): cannot open " << path << std::endl;
		return;
	}

	std::cout << "Game: running exec-file " << path << std::endl;
	std::string line;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos || line[start] == '#')
			continue;
		line = line.substr(start);

		std::cout << "TestScript> " << line << std::endl;

		// A "WAIT-TICKS <n>" line isn't a real console command - it runs
		// n logic ticks before the next line. Console commands only queue
		// an action (Object::AddAction()); it runs immediately only if
		// the game's INSTANT.IDS marks that action id as instant AND the
		// target's action list was empty - otherwise it just sits queued
		// until something ticks logic. Not done automatically after every
		// line: ticking logic also re-runs the current area's own AI
		// scripts (e.g. an in-progress opening cutscene), which can be
		// slow/long-running - so opt in explicitly with WAIT-TICKS right
		// after a command that needs it, rather than paying that cost on
		// every line.
		if (line.compare(0, 10, "WAIT-TICKS") == 0) {
			int ticks = ::atoi(line.c_str() + 10);
			for (int i = 0; i < ticks; i++)
				Core::Get()->UpdateLogic(true);
			continue;
		}

		ExecuteCommand(line);
	}
	std::cout << "Game: exec-file done, quitting" << std::endl;
}
