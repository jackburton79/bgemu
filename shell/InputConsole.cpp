/*
 * InputConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "InputConsole.h"

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


InputConsole::InputConsole(const GFX::rect& rect)
	:
	Console(rect)
{
}


InputConsole::~InputConsole()
{
	std::list<ShellCommand*>::iterator command;
	for (command = fCommands.begin();
			command != fCommands.end(); command++) {
		delete *command;
	}
}


void
InputConsole::Initialize()
{
	AddCommands(this);
}


void
InputConsole::ShowHelp()
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
InputConsole::AddCommand(ShellCommand* command)
{
	fCommands.push_back(command);
}


void
InputConsole::HandleInput(uint8 c)
{
	switch (c) {
		case SDLK_RETURN:
			_ExecuteCommand(fBuffer);
			fBuffer = "";
			ClearScreen();
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
InputConsole::_ExecuteCommand(const std::string& line)
{
	std::string command;
	std::istringstream cmdStream(line);
	cmdStream >> command;
	std::string args;
	try {
		size_t pos = cmdStream.tellg();
		cmdStream.seekg(pos + 1);
		args = cmdStream.str().substr(cmdStream.tellg());
	} catch (...) {
		std::cerr << Log::Red;
		std::cerr << "_ExecuteCommand(): " << line << " failed!" << std::endl;
		std::cerr << Log::Normal;
	}
	std::cout << "Command: " << line << std::endl;
	ShellCommand* shellCommand = _FindCommand(command);
	if (shellCommand != NULL)
		(*shellCommand)(args.c_str(), 1);
	else {
		std::cout << "Invalid Command!" << std::endl;
		ShowHelp();
	}
}


ShellCommand*
InputConsole::_FindCommand(const std::string& cmd)
{
	std::list<ShellCommand*>::iterator i;
	for (i = fCommands.begin();
			i != fCommands.end(); i++) {
		ShellCommand* shellCommand = *i;
		if (shellCommand->Command() == cmd)
			return shellCommand;
	}

	return NULL;
}


std::string
InputConsole::_FindCompleteCommand(const std::string& partialCommand) const
{
	// TODO: Returns only the first matching command
	std::list<ShellCommand*>::const_iterator i;
	for (i = fCommands.begin();
			i != fCommands.end(); i++) {
		std::string fullCmd = (*i)->Command();
		if (fullCmd.compare(0, partialCommand.length(),
				partialCommand) == 0) {
			return fullCmd;
		}
	}
	return partialCommand;
}
