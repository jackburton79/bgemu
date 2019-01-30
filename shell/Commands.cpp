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
#include "Timer.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>


#include "ShellCommand.h"

class ListObjectsCommand : public ShellCommand {
public:
	ListObjectsCommand() : ShellCommand("list-objects") {}
	virtual ~ListObjectsCommand() {};
	virtual void operator()(const char* argv, int argc) {
		ActorsList objects;
		ActorsList::iterator i;
		Core::Get()->GetObjectList(objects);
		for (i = objects.begin(); i != objects.end(); i++) {
			Actor* actor = *i;
			std::cout << actor->Name();
			std::cout << " (" << actor->CRE()->GlobalActorEnum() << ")";
			std::cout << std::endl;
		}
	}

};


// PrintObjectCommand
class PrintObjectCommand : public ShellCommand {
public:
	PrintObjectCommand() : ShellCommand("print-object") {};
	virtual ~PrintObjectCommand() {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		uint32 num;
		Object* object = NULL;
		if ((stringStream >> num).fail())
			object = Core::Get()->GetObject(argv);
		else
			object = Core::Get()->GetObject(num);

		if (object != NULL)
			object->Print();
		else
			std::cout << "object \"" << argv << "\" not found." << std::endl;
	}
};


class ListResourcesCommand : public ShellCommand {
public:
	ListResourcesCommand() : ShellCommand("list-resources") {};
	virtual ~ListResourcesCommand() {};
	virtual void operator()(const char* argv, int argc) {
		StringList stringList;
		gResManager->GetCachedResourcesList(stringList);
		StringListIterator i;
		for (i = stringList.begin(); i != stringList.end(); i++) {
			std::cout << (*i) << std::endl;
		}
	}
};


class WaitTimeCommand : public ShellCommand {
public:
	WaitTimeCommand() : ShellCommand("wait-time") {};
	virtual ~WaitTimeCommand() {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		uint16 hours;
		if (!(stringStream >> hours).fail()) {
			GameTimer::AdvanceTime(hours * 60 * 60);
			GameTimer::PrintTime();
		}
	}
};


class PrintVariablesCommand : public ShellCommand {
public:
	PrintVariablesCommand() : ShellCommand("print-variables") {};
	virtual void operator()(const char* argv, int argc) {
		Core::Get()->PrintVariables();
	}
};

class ExitCommand : public ShellCommand {
public:
	ExitCommand() : ShellCommand("exit") {};
	virtual void operator()(const char* argv, int argc) {
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	};
};

void
AddCommands(InputConsole* console)
{
	console->AddCommand(new ListObjectsCommand());
	console->AddCommand(new PrintObjectCommand());
	console->AddCommand(new ListResourcesCommand());
	console->AddCommand(new WaitTimeCommand());
	console->AddCommand(new PrintVariablesCommand());
	console->AddCommand(new ExitCommand());
}
