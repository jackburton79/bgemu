/*
 * Commands.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */

#include "Commands.h"

#include "Action.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GameConsole.h"
#include "GUI.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"
#include "Timer.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>


#include "ShellCommand.h"

class ListObjectsCommand : public ShellCommand {
public:
	ListObjectsCommand()
		: ShellCommand("List-Objects")
	{
	}
	virtual ~ListObjectsCommand() {};
	virtual void operator()(const char* argv) {
		ActorsList objects;
		ActorsList::iterator i;
		((AreaRoom*)Core::Get()->CurrentRoom())->GetActorsList(objects);
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
	PrintObjectCommand()
		: ShellCommand(
			"Print-Object",
			{
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual ~PrintObjectCommand() {};
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string name = params.at(0).value.string;
		Object* object = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(name.c_str());

		if (object != NULL)
			object->Print();
		else
			std::cout << "object \"" << name << "\" not found." << std::endl;
	}
};


class ListResourcesCommand : public ShellCommand {
public:
	ListResourcesCommand()
		: ShellCommand("List-Resources")
	{
	}
	virtual ~ListResourcesCommand() {};
	virtual void operator()(const char* argv) {
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
	WaitTimeCommand()
		: ShellCommand(
			"Wait-Time",
			{
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual ~WaitTimeCommand() {};
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		uint16 hours = params.at(0).value.integer;
		GameTimer::AdvanceTime(hours * 60 * 60);
		GameTimer::PrintTime();
	}
};


class PrintVariablesCommand : public ShellCommand {
public:
	PrintVariablesCommand()
		: ShellCommand("Print-Variables")
	{
	}
	virtual void operator()(const char* argv) {
		Core::Get()->Vars().PrintAll();
	}
};


class ShowWindowCommand : public ShellCommand {
public:
	ShowWindowCommand()
		: ShellCommand(
			"Toggle-Window",
			{
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		uint16 windowID = params.at(0).value.integer;
		GUI::Get()->ToggleWindow(windowID);
	}
};


#if 0
class WalkToObjectCommand : public ShellCommand {
public:
	WalkToObjectCommand()
		: ShellCommand("Walk-ToObject")
	{
	}
	virtual void operator()(const char* argv) {
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
	MoveViewPointCommand()
		: ShellCommand(
			"Move-ViewPoint",
			{
				{ PARAMETER_POINT, },
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point where = params.at(0).value.point;
		int speed = params.at(1).value.integer;

		action_params* actionParams = new action_params;
		actionParams->integer1 = speed;
		actionParams->where = where;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionMoveViewPoint(room, actionParams);
		room->AddAction(action);
		actionParams->Release();
	}
};


class ShakeScreenCommand : public ShellCommand {
public:
	ShakeScreenCommand()
		: ShellCommand(
			"Shake-Screen",
			{
				{ PARAMETER_POINT, },
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::istringstream stringStream(argv);
		IE::point where = params.at(0).value.point;
		int duration = params.at(1).value.integer;

		action_params* actionParams = new action_params;
		actionParams->integer1 = duration;
		actionParams->where = where;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionScreenShake(room, actionParams);
		room->AddAction(action);
		actionParams->Release();
	}
};


class CreateVisualEffectCommand : public ShellCommand {
public:
	CreateVisualEffectCommand()
		: ShellCommand(
			"Create-VisualEffect",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_POINT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string effectName = params.at(0).value.string;
		IE::point where = params.at(0).value.point;

		action_params* actionParams = new action_params;
		strcpy(actionParams->string1, effectName.c_str());
		actionParams->where = where;

		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionCreateVisualEffect(room, actionParams);
		room->AddAction(action);
		actionParams->Release();
	}
};


class CreateCreatureCommand : public ShellCommand {
public:
	CreateCreatureCommand()
		: ShellCommand(
			"Create-Creature",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_POINT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);

		action_params* actionParams = new action_params;
		strcpy(actionParams->string1, params.at(0).value.string);
		actionParams->where = params.at(1).value.point;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionCreateCreature(room, actionParams);
		room->AddAction(action);
		actionParams->Release();
	}
};


class DestroyCreatureCommand : public ShellCommand {
public:
	DestroyCreatureCommand()
		: ShellCommand(
			"Destroy-Creature",
			{
			   { PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Object* object = NULL;
		// TODO: Use PARAMETER_STRING_OR_INTEGER
		// and reimplement this
		// If an id was passed, use it.
		// otherwise use the passed string (the creature name)
		std::string name = params.at(0).value.string;
		object = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(name.c_str());

		if (object != NULL) {
			action_params* actionParams = new action_params;
			// TODO: we pass a dummy actionParams because Action::IsInstant()
			// crashes if actionParams is NULL
			RoomBase* room = Core::Get()->CurrentRoom();
			Action* action = new ActionDestroySelf(object, actionParams);
			room->AddAction(action);
			actionParams->Release();
		} else
			std::cout << "object \"" << argv << "\" not found." << std::endl;
	}
};


class DisableCreatureCommand : public ShellCommand {
public:
	DisableCreatureCommand()
		: ShellCommand(
			"Disable-Creature",
			{
			   { PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string name = params.at(0).value.string;
		Object* object = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(name.c_str());

		if (object != NULL) {
			object->Disable();
		} else
			std::cout << "object \"" << argv << "\" not found." << std::endl;
	}
};


class DisplayStringCommand : public ShellCommand {
public:
	DisplayStringCommand()
		: ShellCommand("Display-String",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_POINT, },
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string string = params.at(0).value.string;
		IE::point where = params.at(1).value.point;
		int duration = params.at(2).value.integer;

		action_params* actionParams = new action_params;
		RoomBase* room = Core::Get()->CurrentRoom();

		actionParams->integer1 = duration;
		actionParams->where = where;

		// TODO: ActionDisplayString needs an int, not a string
		Action* action = new ActionDisplayString(room, actionParams);
		room->AddAction(action);
	}
};


class ExitCommand : public ShellCommand {
public:
	ExitCommand()
		: ShellCommand("Exit")
	{
	}
	virtual void operator()(const char* argv) {
		SDL_Event event;
		event.type = SDL_QUIT;
		SDL_PushEvent(&event);
	};
};


class SetEnemyAllyCommand : public ShellCommand {
public:
	SetEnemyAllyCommand()
		: ShellCommand(
			"Set-EnemyAlly",
			{
			   { PARAMETER_STRING, },
			   { PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string creatureName = params.at(0).value.string;
		uint16 enemyAlly = params.at(1).value.integer;

		Object* object = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(creatureName.c_str());
		if (object == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->integer1 = enemyAlly;
		RoomBase* room = Core::Get()->CurrentRoom();
		Action* action = new ActionSetEnemyAlly(object, actionParams);
		room->AddAction(action);
		actionParams->Release();
	};
};


void
AddCommands(GameConsole* console)
{
	console->AddCommand(new CreateCreatureCommand());
	console->AddCommand(new CreateVisualEffectCommand());
	console->AddCommand(new DestroyCreatureCommand());
	console->AddCommand(new DisableCreatureCommand());
	console->AddCommand(new ExitCommand());
	console->AddCommand(new ListObjectsCommand());
	console->AddCommand(new ListResourcesCommand());
	console->AddCommand(new MoveViewPointCommand());
	console->AddCommand(new PrintObjectCommand());
	console->AddCommand(new PrintVariablesCommand());
	console->AddCommand(new SetEnemyAllyCommand());
	console->AddCommand(new ShowWindowCommand());
	console->AddCommand(new ShakeScreenCommand());
	console->AddCommand(new WaitTimeCommand());
#if 0
	console->AddCommand(new WalkToObjectCommand());
	console->AddCommand(new DisplayStringCommand());
#endif
}
