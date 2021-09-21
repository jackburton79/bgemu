/*
 * Commands.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "Commands.h"

#include "Action.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "InputConsole.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"
#include "RoomBase.h"
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
		Core::Get()->GetActorsList(objects);
		for (i = objects.begin(); i != objects.end(); i++) {
			Actor* actor = *i;
			std::cout << actor->Name();
			std::cout << " (" << std::dec << actor->CRE()->GlobalActorEnum() << ")";
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
		Core::Get()->Vars().PrintAll();
	}
};


class ShowWindowCommand : public ShellCommand {
public:
	ShowWindowCommand() : ShellCommand("toggle-window") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		uint16 windowID;
		if (!(stringStream >> windowID).fail()) {
			GUI::Get()->ToggleWindow(windowID);
		}
	}
};


#if 0
class WalkToObjectCommand : public ShellCommand {
public:
	WalkToObjectCommand() : ShellCommand("walk-to-object") {};
	virtual void operator()(const char* argv, int argc) {
		int objectId = 0;
		std::istringstream stringStream(argv);
		if ((stringStream >> objectId).fail())
			return;
		// TODO: Fix this
		Actor* player = Game::Get()->Party()->ActorAt(0);
		if (player == NULL)
			return;
		Action* action = new WalkToObject(player, Core::Get()->GetObject(objectId));
		player->AddAction(action);
	}
};


class MoveViewPointCommand : public ShellCommand {
public:
	MoveViewPointCommand() : ShellCommand("move-viewpoint") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		IE::point where;
		int speed;
		char o;
		stringStream >> where.x >> o >> where.y >> o >> speed;
		std::cout << std::dec << where.x << ", " << where.y << ". Speed " << speed << std::endl;
		Action* action = new MoveViewPoint(NULL, where, speed);
		Core::Get()->AddGlobalAction(action);
	}
};
#endif

class ShakeScreenCommand : public ShellCommand {
public:
	ShakeScreenCommand() : ShellCommand("shake-screen") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		IE::point where;
		int duration;
		char o;
		stringStream >> where.x >> o >> where.y >> o >> duration;
		// TODO: We are leaking the actionParams
		action_node* actionParams = new action_node;
		//strcpy(actionParams->first.name, actor->Name());
		//strcpy(actionParams->second.name, Name());
		actionParams->integer1 = duration;
		actionParams->where = where;
		std::cout << "creating action" << std::endl;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionScreenShake(room, actionParams);
		std::cout << "Adding action" << std::endl;
		room->AddAction(action);
		std::cout << "end" << std::endl;
	}
};

#if 0
class DisplayStringCommand : public ShellCommand {
public:
	DisplayStringCommand() : ShellCommand("display-string") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		std::string string;
		IE::point where;
		char o;
		int duration;
		stringStream >> string >> o >> where.x >> o >> where.y >> o >> duration;
		Action* action = new DisplayString(NULL, string.c_str(), where, duration);
		Core::Get()->AddGlobalAction(action);
	}
};

#endif
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
	console->AddCommand(new ShowWindowCommand());
	console->AddCommand(new ExitCommand());
	console->AddCommand(new ShakeScreenCommand());
#if 0
	console->AddCommand(new WalkToObjectCommand());
	console->AddCommand(new MoveViewPointCommand());
	console->AddCommand(new DisplayStringCommand());
#endif
}
