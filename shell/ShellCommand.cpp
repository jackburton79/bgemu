/*
 * ShellCommand.cpp
 *
 *  Created on: 31/mar/2015
 *      Author: stefano
 */

#include "ShellCommand.h"

ShellCommand::ShellCommand(const char* command)
	:
	fCommand(command)
{
}


ShellCommand::~ShellCommand()
{
}


std::string
ShellCommand::Command() const
{
	return fCommand;
}
