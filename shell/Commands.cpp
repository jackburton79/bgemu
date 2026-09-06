/*
 * Commands.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */


#include "Commands.h"

#include "AreaRoom.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "Dialog.h"
#include "Game.h"
#include "GameConsole.h"
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"

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


// ListContainersCommand - dumps container names in the current area
// (List-Objects above only covers actors) so container-click tests can
// find a real target name without guessing.
class ListContainersCommand : public ShellCommand {
public:
	ListContainersCommand()
		: ShellCommand("List-Containers")
	{
	}
	virtual void operator()(const char* argv) {
		for (Container* container : ((AreaRoom*)Core::Get()->CurrentRoom())->Containers())
			std::cout << container->Name() << std::endl;
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


// PrintJournalCommand - dumps Game's minimal in-memory journal (see
// Game::AddJournalEntry()'s header comment) so ADDJOURNALENTRY/
// ERASEJOURNALENTRY/SETQUESTDONE are testable headlessly.
class PrintJournalCommand : public ShellCommand {
public:
	PrintJournalCommand()
		: ShellCommand("Print-Journal")
	{
	}
	virtual void operator()(const char* argv) {
		for (uint32 strref : Game::Get()->JournalEntries())
			std::cout << strref << ": " << IDTable::GetDialog(strref) << std::endl;
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


class ToggleAuxWindowCommand : public ShellCommand {
public:
	ToggleAuxWindowCommand()
		: ShellCommand(
			"Toggle-AuxWindow",
			{
				{ PARAMETER_STRING, }, // CHU resource name, e.g. GUIINV
				{ PARAMETER_INT, }     // window id within it
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		res_ref chuName(params.at(0).value.string);
		uint16 windowID = params.at(1).value.integer;
		GUI::Get()->ToggleAuxWindow(chuName, windowID);
		std::cout << "Toggle-AuxWindow: " << chuName.CString() << " window " << windowID
				<< " shown=" << GUI::Get()->IsAuxWindowShown(chuName, windowID) << std::endl;
	}
};


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
		action_params* actionParams = new action_params();
		actionParams->id = 22; // MoveToObject
		actionParams->integer1 = objectId;
		player->AddAction(actionParams);
		actionParams->Release();
	}
};


// ClickObjectCommand - headless equivalent of clicking a world object
// with the mouse (see AreaRoom::MouseDown(), which resolves the object
// under the cursor and calls this same Object::ClickedOn() on the
// clicking actor) - lets exec-file tests exercise click-driven behavior
// (attack/dialog/door-open routing) without a real screen click.
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
		actionParams->id = 49; // MoveViewPoint
		RoomBase* room = Core::Get()->CurrentRoom();
		room->AddAction(actionParams);
		actionParams->Release();
	}
};


// ScreenshotCommand - renders one frame (GUI::Draw(), same call the
// normal interactive loop makes every frame - see Game::Loop() -
// exec-file mode never reaches that loop, so nothing gets drawn into
// GraphicsEngine's software surface otherwise) and dumps it to a BMP
// file (GraphicsEngine::SaveScreenshot()). Works under SDL_VIDEODRIVER=
// dummy (used for headless ASan test runs all along - the software
// surface everything is drawn into is video-driver-independent, only
// actually presenting it to a real screen isn't) - lets a GUI/visual
// change be inspected from an exec-file test without a real display.
class ScreenshotCommand : public ShellCommand {
public:
	ScreenshotCommand()
		: ShellCommand(
			"Screenshot",
			{
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		GUI::Get()->Draw();
		bool ok = GraphicsEngine::Get()->SaveScreenshot(params.at(0).value.string);
		std::cout << "Screenshot: " << (ok ? "OK" : "FAILED") << std::endl;
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
		actionParams->id = 254; // ScreenShake
		RoomBase* room = Core::Get()->CurrentRoom();
		room->AddAction(actionParams);
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
		actionParams->id = 272;
		RoomBase* room = Core::Get()->CurrentRoom();
		room->AddAction(actionParams);
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
		actionParams->id = 7; // CreateCreature
		RoomBase* room = Core::Get()->CurrentRoom();
		room->AddAction(actionParams);
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
			actionParams->id = 111; //DestroySelf
			object->AddAction(actionParams);
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
		room->AddAction(actionParams);
		actionParams->Release();
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


// Resolves a script name to an Actor* in the current room, printing a
// "not found" message and returning NULL if it doesn't exist or isn't an
// actor. Shared by every command below that takes an actor name.
static Actor*
FindActor(const std::string& name)
{
	Object* object = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(name.c_str());
	Actor* actor = dynamic_cast<Actor*>(object);
	if (actor == NULL)
		std::cout << "actor \"" << name << "\" not found." << std::endl;
	return actor;
}


// ClickObjectCommand - headless equivalent of clicking a world object
// with the mouse (see AreaRoom::MouseDown(), which resolves the object
// under the cursor and calls this same Object::ClickedOn() on the
// clicking actor) - lets exec-file tests exercise click-driven behavior
// (attack/dialog/door-open routing) without a real screen click.
class ClickObjectCommand : public ShellCommand {
public:
	ClickObjectCommand()
		: ShellCommand(
			"Click-Object",
			{
				{ PARAMETER_STRING, }, // clicking actor
				{ PARAMETER_STRING, }  // target object name
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		std::string targetName = params.at(1).value.string;
		Object* target = ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(targetName.c_str());
		if (target == NULL) {
			std::cout << "Click-Object: target \"" << targetName << "\" not found." << std::endl;
			return;
		}
		actor->ClickedOn(target);
	}
};


class GiveItemCommand : public ShellCommand {
public:
	GiveItemCommand()
		: ShellCommand(
			"Give-Item",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_STRING, },
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		// Routes through the real CREATEITEM(82) action dispatch
		// (scripting/Actions.cpp) rather than calling Actor::AddItem()
		// directly, so this also exercises the same path real scripts use.
		action_params* actionParams = new action_params;
		actionParams->id = 82; // CreateItem
		strcpy(actionParams->string1, params.at(1).value.string);
		actionParams->integer1 = params.at(2).value.integer;
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


class EquipItemCommand : public ShellCommand {
public:
	EquipItemCommand()
		: ShellCommand(
			"Equip-Item",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = 11; // EquipItem
		strcpy(actionParams->string1, params.at(1).value.string);
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


class DropItemCommand : public ShellCommand {
public:
	DropItemCommand()
		: ShellCommand(
			"Drop-Item",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = 9; // DropItem
		strcpy(actionParams->string1, params.at(1).value.string);
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


class PrintInventoryCommand : public ShellCommand {
public:
	PrintInventoryCommand()
		: ShellCommand(
			"Print-Inventory",
			{
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		std::cout << std::dec << "Inventory for " << actor->Name() << ":" << std::endl;
		for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
			IE::item item;
			if (actor->CRE()->GetItemAtSlot(slot, item)) {
				std::cout << "  slot " << std::dec << slot << ": " << item.name.CString()
						<< " x" << item.quantity1 << std::endl;
			}
		}
	}
};


class AttackCommand : public ShellCommand {
public:
	AttackCommand()
		: ShellCommand(
			"Attack",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = 3; // Attack
		strcpy(actionParams->Second()->name, params.at(1).value.string);
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


class ForceSpellCommand : public ShellCommand {
public:
	ForceSpellCommand()
		: ShellCommand(
			"Force-Spell",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_STRING, },
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = 113; // ForceSpell
		strcpy(actionParams->Second()->name, params.at(1).value.string);
		actionParams->integer1 = params.at(2).value.integer; // SPELL.IDS id
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


// General-purpose escape hatch: queues an arbitrary action id on a named
// actor, with an optional named target, integer1 and string1 - for
// testing action ids that don't have (and don't need) their own dedicated
// command above. See scripting/Actions.cpp's kActionsTable for ids.
class RunActionCommand : public ShellCommand {
public:
	RunActionCommand()
		: ShellCommand(
			"Run-Action",
			{
				{ PARAMETER_STRING, }, // actor
				{ PARAMETER_INT, },    // action id
				{ PARAMETER_STRING, }, // target name (may be empty, "-")
				{ PARAMETER_INT, },    // integer1
				{ PARAMETER_STRING, }, // string1 (may be empty, "-")
				{ PARAMETER_STRING, }, // string2 (may be empty, "-")
				{ PARAMETER_POINT, }   // where (x,y - use 0,0 if unused)
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Actor* actor = FindActor(params.at(0).value.string);
		if (actor == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = params.at(1).value.integer;
		std::string target = params.at(2).value.string;
		if (!target.empty() && target != "-")
			strcpy(actionParams->Second()->name, target.c_str());
		actionParams->integer1 = params.at(3).value.integer;
		std::string string1 = params.at(4).value.string;
		if (string1 != "-")
			strcpy(actionParams->string1, string1.c_str());
		std::string string2 = params.at(5).value.string;
		if (string2 != "-")
			strcpy(actionParams->string2, string2.c_str());
		actionParams->where = params.at(6).value.point;
		actor->AddAction(actionParams);
		actionParams->Release();
	}
};


class SaveGameCommand : public ShellCommand {
public:
	SaveGameCommand()
		: ShellCommand(
			"Save-Game",
			{
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		bool ok = Game::Get()->Save(params.at(0).value.string);
		std::cout << "Save-Game: " << (ok ? "OK" : "FAILED") << std::endl;
	}
};


class LoadGameCommand : public ShellCommand {
public:
	LoadGameCommand()
		: ShellCommand(
			"Load-Game",
			{
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		bool ok = Game::Get()->Load(params.at(0).value.string);
		std::cout << "Load-Game: " << (ok ? "OK" : "FAILED") << std::endl;
	}
};


// SelectDialogOptionCommand - headless equivalent of clicking a line in
// the dialog TextArea (see TextArea::MouseDown()'s logic, mirrored here
// exactly): pass a 0-based player-response index while the dialog is
// waiting for one (see the "Response N:" lines DialogHandler now prints
// to stdout - see Dialog.cpp), or -1 to just click through NPC text that
// has no response options ("click to continue"). Terminates the dialog
// (same as TerminateDialog()) once Continue() reports it has ended.
class SelectDialogOptionCommand : public ShellCommand {
public:
	SelectDialogOptionCommand()
		: ShellCommand(
			"Select-DialogOption",
			{
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		int32 option = params.at(0).value.integer;

		DialogHandler* dialog = Game::Get()->Dialog();
		if (dialog == NULL) {
			std::cout << "Select-DialogOption: no active dialog" << std::endl;
			return;
		}

		if (dialog->IsWaitingUserChoice()) {
			if (option < 0) {
				std::cout << "Select-DialogOption: dialog is waiting for "
					"a player choice, pass an option index >= 0" << std::endl;
				return;
			}
			dialog->SelectOption(option);
		}

		if (!dialog->Continue())
			Game::Get()->TerminateDialog();
	}
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
		object->AddAction(actionParams);
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
	console->AddCommand(new ListContainersCommand());
	console->AddCommand(new ListResourcesCommand());
	console->AddCommand(new MoveViewPointCommand());
	console->AddCommand(new PrintObjectCommand());
	console->AddCommand(new PrintVariablesCommand());
	console->AddCommand(new PrintJournalCommand());
	console->AddCommand(new SetEnemyAllyCommand());
	console->AddCommand(new ShowWindowCommand());
	console->AddCommand(new ToggleAuxWindowCommand());
	console->AddCommand(new ShakeScreenCommand());
	console->AddCommand(new ScreenshotCommand());
	console->AddCommand(new WaitTimeCommand());

	console->AddCommand(new WalkToObjectCommand());
	console->AddCommand(new ClickObjectCommand());
	console->AddCommand(new DisplayStringCommand());

	console->AddCommand(new GiveItemCommand());
	console->AddCommand(new EquipItemCommand());
	console->AddCommand(new DropItemCommand());
	console->AddCommand(new PrintInventoryCommand());
	console->AddCommand(new AttackCommand());
	console->AddCommand(new ForceSpellCommand());
	console->AddCommand(new RunActionCommand());
	console->AddCommand(new SaveGameCommand());
	console->AddCommand(new LoadGameCommand());
	console->AddCommand(new SelectDialogOptionCommand());
}
