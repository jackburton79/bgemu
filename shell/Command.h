/*
 * Command.h
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#ifndef COMMAND_H_
#define COMMAND_H_

class ShellContext;
class Command {
public:
	Command();
	~Command();
	void ParseAndExecute();

private:
	ShellContext* fContext;
};

#endif /* COMMAND_H_ */
