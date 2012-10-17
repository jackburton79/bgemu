/*
 * DoorContext.h
 *
 *  Created on: 08/ott/2012
 *      Author: stefano
 */

#ifndef __DOORCONTEXT_H_
#define __DOORCONTEXT_H_

#include "ShellContext.h"

class Door;
class DoorContext : public ShellContext {
public:
	DoorContext(ShellContext* parent, Door* door);
	~DoorContext();
	virtual ShellContext* HandleCommand(std::string command);
	virtual void PrintPrompt();
	virtual void ListCommands();

private:
	Door* fDoor;
};

#endif /* DOORCONTEXT_H_ */
