/*
 * InputConsole.h
 *
 *  Created on: 13/ott/2012
 *      Author: stefano
 */

#ifndef INPUTCONSOLE_H_
#define INPUTCONSOLE_H_

#include "Console.h"

#include <string>

class ShellContext;
class InputConsole: public Console {
public:
	InputConsole(const GFX::rect& rect);
	~InputConsole();

	void HandleInput(uint8 key);
private:
	void _ExecuteCommand(std::string str);

	ShellContext* fContext;
	std::string fBuffer;
};

#endif /* INPUTCONSOLE_H_ */
