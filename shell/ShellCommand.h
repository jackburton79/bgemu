/*
 * ShellCommand.h
 *
 *  Created on: 31/mar/2015
 *      Author: stefano
 */

#ifndef SHELLCOMMAND_H_
#define SHELLCOMMAND_H_

#include <string>
#include <vector>

#include "IETypes.h"

enum ShellCommandParameterType {
	PARAMETER_INT = 0,
	PARAMETER_STRING = 1,
	PARAMETER_POINT = 2,
	PARAMETER_INT_OR_STRING = 3
};

struct ShellCommandParameter {
	int type;
	union {
		int integer;
		IE::point point;
		char string[128];
	} value;
};

typedef std::vector<ShellCommandParameter> ShellCommandParameters;

class ShellCommand {
public:
	ShellCommand(const char* command);
	ShellCommand(const char* command, ShellCommandParameters params);
	virtual ~ShellCommand();

	std::string Command() const;

	virtual void operator()(const char* argv, int argc) = 0;

protected:
	void ParseParameters(const char* argv, int argc);
	ShellCommandParameters& Parameters();

private:
	std::string fCommand;
	ShellCommandParameters fParameters;
};


#endif /* SHELLCOMMAND_H_ */
