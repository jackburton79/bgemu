/*
 * Command.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "Command.h"
#include "ResManager.h"
#include "ShellContext.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>


Command::Command()
	:
	fContext(NULL)
{
	fContext = new BaseContext();
}


Command::~Command()
{
	delete fContext;
}


void
Command::ParseAndExecute()
{
	fContext->PrintPrompt();

	char line[256];
	std::cin.getline(line, sizeof(line));

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
