/*
 * ShellCommand.h
 *
 *  Created on: 31/mar/2015
 *      Author: stefano
 */

#ifndef SHELLCOMMAND_H_
#define SHELLCOMMAND_H_

#include <string>

class ShellCommand {
public:
	ShellCommand(const char* command);
	virtual ~ShellCommand();

	std::string Command() const;

	virtual void operator()(const char* argv, int argc) = 0;

private:
	std::string fCommand;
};


#endif /* SHELLCOMMAND_H_ */
