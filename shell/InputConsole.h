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
	void _ExecuteCommand(std::string str);
	ShellCommand* _FindCommand(std::string cmd);
	std::string _FindCompleteCommand(std::string partialCommand);

	std::string fBuffer;

	std::list<ShellCommand*> fCommands;
};

#endif /* INPUTCONSOLE_H_ */
