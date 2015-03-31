/*
 * InputConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "Commands.h"
#include "InputConsole.h"
#include "ResManager.h"
#include "ShellCommand.h"

#include "SDL.h"

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
InputConsole::AddCommand(ShellCommand* command)
{
	fCommands.push_back(command);
}


void
InputConsole::HandleInput(uint8 c)
{
	if (c == SDLK_RETURN) {
		_ExecuteCommand(fBuffer.c_str());
		fBuffer = "";
		ClearScreen();
	} else {
		fBuffer.push_back(c);
		_ParseCharacter(c);
	}
}


void
InputConsole::_ExecuteCommand(std::string line)
{
	std::string text;
	std::istringstream cmdString(line);
	cmdString >> text;

	std::list<ShellCommand*>::iterator command;
	for (command = fCommands.begin();
			command != fCommands.end(); command++) {
		ShellCommand& cmd = *(*command);
		if (cmd.Command() == text) {
			cmd("", 1);
			break;
		}
	}
}
