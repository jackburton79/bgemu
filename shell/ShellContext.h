/*
 * ShellContext.h
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#ifndef SHELLCONTEXT_H_
#define SHELLCONTEXT_H_

#include "IETypes.h"

#include <string>

class ShellContext {
public:
	ShellContext(ShellContext* parent);
	virtual ShellContext* HandleCommand(std::string command) = 0;
	virtual void PrintPrompt() = 0;
	virtual void ListCommands() = 0;
	virtual ~ShellContext();

	static ShellContext* ExitContext(ShellContext* context);
protected:
	ShellContext* fParentContext;
};


class BaseContext : public ShellContext {
public:
	BaseContext();
	virtual ShellContext* HandleCommand(std::string command);
	virtual void PrintPrompt();
	virtual void ListCommands();
	virtual ~BaseContext();
};



#endif /* SHELLCONTEXT_H_ */
