/*
 * InputConsole.cpp
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#include "InputConsole.h"
#include "ResManager.h"
#include "ShellContext.h"

InputConsole::InputConsole(const GFX::rect& rect)
	:
	Console(rect)
{
	fContext = new BaseContext();
}


InputConsole::~InputConsole()
{
	delete fContext;
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

	// TODO: Change this, it doesn't look nice!

	if (!text.compare("exit")) {
		fContext = ShellContext::ExitContext(fContext);
		if (fContext == NULL)
			exit(0);
	} else if (!text.compare("list")) {
		std::string stringParam;
		cmdString >> stringParam;
		if (!stringParam.compare("resources")) {
			gResManager->PrintResources();
		} else
			fContext->HandleCommand(line);
	} else if (!text.compare("help")) {
		fContext->ListCommands();
	} else
		fContext = fContext->HandleCommand(line);

}
