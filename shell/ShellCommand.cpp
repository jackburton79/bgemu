/*
 * ShellCommand.cpp
 *
 *  Created on: 31/mar/2015
 *      Author: stefano
 */

#include "ShellCommand.h"

#include <cstdlib>
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
			{
				// Bounded, not strcpy(): value.string is a fixed
				// 128-byte buffer (a union member, so it can't just be
				// a std::string) - an unbounded copy overflows it for
				// any parameter longer than that (found via a real
				// crash: a scratchpad screenshot path a few characters
				// over 128). Truncates instead of overflowing; long
				// enough for every real use of this console (resrefs,
				// object names, file paths in practice).
				const std::string& value = strings.at(p++);
				size_t maxLen = sizeof(fParameters[i].value.string) - 1;
				strncpy(fParameters[i].value.string, value.c_str(), maxLen);
				fParameters[i].value.string[maxLen] = '\0';
				break;
			}
			case PARAMETER_INT:
				fParameters[i].value.integer = ::strtoul(strings.at(p++).c_str(), NULL, 0);
				break;
			default:
				break;
		}
	}

	return fParameters;
}
