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
	ListObjectsCommand() : ShellCommand("List-Objects") {}
	virtual ~ListObjectsCommand() {};
	virtual void operator()(const char* argv, int argc) {
		/*ActorsList objects;
		ActorsList::iterator i;
		Core::Get()->GetActorsList(objects);
		for (i = objects.begin(); i != objects.end(); i++) {
			Actor* actor = *i;
			std::cout << actor->Name();
			std::cout << " (" << std::dec << actor->CRE()->GlobalActorEnum() << ")";
			std::cout << std::endl;
		}*/
	}
};


// PrintObjectCommand
class PrintObjectCommand : public ShellCommand {
public:
	PrintObjectCommand() : ShellCommand("Print-Object") {};
	virtual ~PrintObjectCommand() {};
	virtual void operator()(const char* argv, int argc) {
		/*std::istringstream stringStream(argv);
		uint32 num;
		Object* object = NULL;
		if ((stringStream >> num).fail())
			object = Core::Get()->GetObject(argv);
		else
			object = Core::Get()->GetObject(num);

		if (object != NULL)
			object->Print();
		else
			std::cout << "object \"" << argv << "\" not found." << std::endl;*/
	}
};


class ListResourcesCommand : public ShellCommand {
public:
	ListResourcesCommand() : ShellCommand("List-Resources") {};
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
	WaitTimeCommand() : ShellCommand("Wait-Time") {};
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
	PrintVariablesCommand() : ShellCommand("Print-Variables") {};
	virtual void operator()(const char* argv, int argc) {
		Core::Get()->Vars().PrintAll();
	}
};


class ShowWindowCommand : public ShellCommand {
public:
	ShowWindowCommand() : ShellCommand("Toggle-Window") {};
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
	WalkToObjectCommand() : ShellCommand("Walk-ToObject") {};
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
#endif


class MoveViewPointCommand : public ShellCommand {
public:
	MoveViewPointCommand() : ShellCommand("Move-ViewPoint") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		IE::point where;
		int speed;
		char o;

		stringStream >> where.x >> o >> where.y >> o >> speed;
		// TODO: We are leaking the actionParams
		action_params* actionParams = new action_params;
		actionParams->integer1 = speed;
		actionParams->where = where;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionMoveViewPoint(room, actionParams);
		room->AddAction(action);
	}
};


class ShakeScreenCommand : public ShellCommand {
public:
	ShakeScreenCommand() : ShellCommand("Shake-Screen") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		IE::point where;
		int duration;
		char o;
		stringStream >> where.x >> o >> where.y >> o >> duration;
		// TODO: We are leaking the actionParams
		action_params* actionParams = new action_params;
		actionParams->integer1 = duration;
		actionParams->where = where;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionScreenShake(room, actionParams);
		room->AddAction(action);
	}
};


class CreateVisualEffectCommand : public ShellCommand {
public:
	CreateVisualEffectCommand() : ShellCommand("Create-VisualEffect") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		std::string effectName;
		IE::point where;
		std::getline(stringStream, effectName, ',');
		char o;
		stringStream >> where.x >> o >> where.y;
		// TODO: We are leaking the actionParams
		action_params* actionParams = new action_params;
		strcpy(actionParams->string1, effectName.c_str());
		actionParams->where = where;

		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionCreateVisualEffect(room, actionParams);
		room->AddAction(action);
	}
};


class CreateCreatureCommand : public ShellCommand {
public:
	CreateCreatureCommand() : ShellCommand("Create-Creature") {};
	virtual void operator()(const char* argv, int argc) {
		std::istringstream stringStream(argv);
		std::string creatureName;
		stringStream >> creatureName;
		// TODO: We are leaking the actionParams
		action_params* actionParams = new action_params;
		strcpy(actionParams->string1, creatureName.c_str());
		IE::point where;
		// TODO:
		where.x = 100;
		where.y = 100;
		actionParams->where = where;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionCreateCreature(room, actionParams);
		room->AddAction(action);
	}
};


#if 0
class DisplayStringCommand : public ShellCommand {
public:
	DisplayStringCommand() : ShellCommand("Display-String") {};
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
	ExitCommand() : ShellCommand("Exit") {};
	virtual void operator()(const char* argv, int argc) {
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	};
};


void
AddCommands(InputConsole* console)
{
	console->AddCommand(new CreateCreatureCommand());
	console->AddCommand(new CreateVisualEffectCommand());
	console->AddCommand(new ExitCommand());
	console->AddCommand(new ListObjectsCommand());
	console->AddCommand(new ListResourcesCommand());
	console->AddCommand(new MoveViewPointCommand());
	console->AddCommand(new PrintObjectCommand());
	console->AddCommand(new PrintVariablesCommand());
	console->AddCommand(new ShowWindowCommand());
	console->AddCommand(new ShakeScreenCommand());
	console->AddCommand(new WaitTimeCommand());
#if 0
	console->AddCommand(new WalkToObjectCommand());
	console->AddCommand(new DisplayStringCommand());
#endif
}
