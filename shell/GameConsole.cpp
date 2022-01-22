/*
 * GameConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "GameConsole.h"

#include "Commands.h"
#include "Log.h"
#include "ResManager.h"
#include "ShellCommand.h"

#include <string>

struct CommandSorter {
	bool operator()(ShellCommand* a, ShellCommand* b) const {
		return a->Command() < b->Command();
	}
};


GameConsole::GameConsole(const GFX::rect& rect, bool redirect)
	:
	Console(rect),
	fOldBuf(NULL),
	fOutputRedirected(false),
	fQuit(false)
{
	if (redirect)
		_EnableOutputRedirect();
	fLock = SDL_CreateMutex();
	fThread = SDL_CreateThread(_UpdateFunction, "ConsoleThread", this);
}


GameConsole::~GameConsole()
{
	fQuit = true;
	SDL_WaitThread(fThread, NULL);
	SDL_DestroyMutex(fLock);

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
			//ClearScreen();
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
		if (SDL_LockMutex(console->fLock) == 0) {
			console->Update();
			SDL_Delay(50);
			SDL_UnlockMutex(console->fLock);
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
