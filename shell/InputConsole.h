/*
 * InputConsole.h
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#ifndef INPUTCONSOLE_H_
#define INPUTCONSOLE_H_

#include "Console.h"

#include <list>
#include <string>

class ShellCommand;
class InputConsole: public Console {
public:
	InputConsole(const GFX::rect& rect);
	~InputConsole();

	void Initialize();
	void AddCommand(ShellCommand* command);
	void ShowHelp();
	void HandleInput(uint8 key);

private:
	void _ExecuteCommand(const std::string& str);
	ShellCommand* _FindCommand(const std::string& cmd);
	std::string _FindCompleteCommand(const std::string& partialCommand) const;

	std::string fBuffer;

	std::list<ShellCommand*> fCommands;
};

#endif /* INPUTCONSOLE_H_ */
