/*
 * ShellCommand.cpp
 *
 *  Created on: 31/mar/2015
 *      Author: stefano
 */

#include "ShellCommand.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

ShellCommand::ShellCommand(const char* command)
	:
	fCommand(command)
{
}


ShellCommand::ShellCommand(const char* command, ShellCommandParameters params)
	:
	fCommand(command),
	fParameters(params)
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


const ShellCommandParameters&
ShellCommand::ParseParameters(const char* argv)
{
	fParameters.clear();

	std::istringstream stringStream(argv);
	std::vector<std::string> strings;
	std::string arg;

	while (std::getline(stringStream, arg, ','))
		strings.push_back(arg);

	size_t p = 0;
	for (size_t i = 0; i < fParameters.size(); i++) {
		switch (fParameters[i].type) {
			case PARAMETER_POINT:
				fParameters[i].value.point.x = ::strtoul(strings.at(p++).c_str(), NULL, 0);
				fParameters[i].value.point.y = ::strtoul(strings.at(p++).c_str(), NULL, 0);
				break;
			case PARAMETER_STRING:
				strcpy(fParameters[i].value.string, strings.at(p++).c_str());
				break;
			case PARAMETER_INT:
				fParameters[i].value.integer = ::strtoul(strings.at(p++).c_str(), NULL, 0);
				break;
			default:
				break;
		}
	}
	
	return fParameters;
}
