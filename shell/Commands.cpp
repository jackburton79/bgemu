/*
 * Command.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "Commands.h"
#include "Core.h"
#include "CreResource.h"
#include "InputConsole.h"
#include "ResManager.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>


#include "ShellCommand.h"

class ListObjectsCommand : public ShellCommand {
public:
	ListObjectsCommand()
		:
		ShellCommand("list-objects")
	{
	}
	virtual ~ListObjectsCommand() {};
	virtual void operator()(const char* argv, int argc)
	{
		std::list<Reference<Object> > objects;
		std::list<Reference<Object> >::iterator i;
		Core::Get()->GetObjectList(objects);
		for (i = objects.begin(); i != objects.end(); i++) {
			std::cout << i->Target()->Name();
			Actor* actor = dynamic_cast<Actor*>(i->Target());
			if (actor != NULL) {
				std::cout << " (" << actor->CRE()->GlobalActorEnum() << ")";
			}
			std::cout << std::endl;
		}
	}

};


class PrintObjectCommand : public ShellCommand {
public:
	PrintObjectCommand() : ShellCommand("print-object") {};
	virtual ~PrintObjectCommand() {};
	virtual void operator()(const char* argv, int argc)
	{
		Object* object = Core::Get()->GetObject(argv);
		if (object != NULL)
			object->Print();
		else
			std::cout << "object \"" << argv << "\" not found." << std::endl;
	}
};


void
AddCommands(InputConsole* console)
{
	console->AddCommand(new ListObjectsCommand());
	console->AddCommand(new PrintObjectCommand());
}
