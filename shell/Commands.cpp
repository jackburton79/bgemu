/*
 * Commands.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */


#include "Commands.h"

#include "AreaRoom.h"
#include "Container.h"
#include "Control.h"
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
#include "Script.h"
#include "SearchMap.h"
#include "Window.h"

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


// WAIT-TICKS (in an exec-file) only calls Core::UpdateLogic(), never
// GUI::Draw() - fine for most tests, but useless for reproducing a bug
// that only shows up on a Draw() that happens to land right on a
// particular tick (e.g. the very first frame after an area change), since
// WAIT-TICKS N followed by a single Screenshot draws only once, after
// all N ticks already ran. Step-Ticks interleaves UpdateLogic()+Draw()
// per tick, same as the real interactive loop (see Game::Loop()) -
// draws every single tick in the given range instead of just the last.
class StepTicksCommand : public ShellCommand {
public:
	StepTicksCommand()
		: ShellCommand(
			"Step-Ticks",
			{
				{ PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		int32 ticks = params.at(0).value.integer;
		for (int32 i = 0; i < ticks; i++) {
			Core::Get()->UpdateLogic(true);
			GUI::Get()->Draw();
		}
		std::cout << "Step-Ticks: OK (" << ticks << " ticks)" << std::endl;
	}
};


class ToggleInventoryCommand : public ShellCommand {
public:
	ToggleInventoryCommand()
		: ShellCommand("Toggle-Inventory")
	{
	}
	virtual void operator()(const char* argv) {
		// Test-only equivalent of the SDLK_i handler - exec-file mode
		// never runs the real SDL event loop, so there's no other way to
		// exercise Game::ToggleInventoryWindow() (and the item-icon
		// population it triggers) from a headless script.
		Game::Get()->ToggleInventoryWindow();
		std::cout << "Toggle-Inventory: OK" << std::endl;
	}
};


class ToggleRecordCommand : public ShellCommand {
public:
	ToggleRecordCommand()
		: ShellCommand("Toggle-Record")
	{
	}
	virtual void operator()(const char* argv) {
		Game::Get()->ToggleRecordWindow();
		std::cout << "Toggle-Record: OK" << std::endl;
	}
};


class SelectPartyCommand : public ShellCommand {
public:
	SelectPartyCommand()
		: ShellCommand(
			"Select-Party",
			{
				{ PARAMETER_INT, } // 0-based party member index
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		// Test-only equivalent of the SDLK_1..6 handlers - exec-file mode
		// never runs the real SDL event loop, so there's no other way to
		// exercise Game::SelectPartyMember() from a headless script.
		const ShellCommandParameters params = ParseParameters(argv);
		int32 index = params.at(0).value.integer;
		if (index < 0) {
			std::cout << "Select-Party: index must be >= 0" << std::endl;
			return;
		}
		Game::Get()->SelectPartyMember((uint16)index);
		std::cout << "Select-Party: OK" << std::endl;
	}
};


class ToggleSaveCommand : public ShellCommand {
public:
	ToggleSaveCommand()
		: ShellCommand("Toggle-Save")
	{
	}
	virtual void operator()(const char* argv) {
		Game::Get()->ToggleSaveWindow();
		std::cout << "Toggle-Save: OK" << std::endl;
	}
};


class ToggleLoadCommand : public ShellCommand {
public:
	ToggleLoadCommand()
		: ShellCommand("Toggle-Load")
	{
	}
	virtual void operator()(const char* argv) {
		Game::Get()->ToggleLoadWindow();
		std::cout << "Toggle-Load: OK" << std::endl;
	}
};


class ToggleJournalCommand : public ShellCommand {
public:
	ToggleJournalCommand()
		: ShellCommand("Toggle-Journal")
	{
	}
	virtual void operator()(const char* argv) {
		Game::Get()->ToggleJournalWindow();
		std::cout << "Toggle-Journal: OK" << std::endl;
	}
};


class InvokeControlCommand : public ShellCommand {
public:
	InvokeControlCommand()
		: ShellCommand(
			"Invoke-Control",
			{
				{ PARAMETER_STRING, }, // CHU name (e.g. GUISAVE)
				{ PARAMETER_INT, },    // window id
				{ PARAMETER_INT, }     // control id
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		// Test-only equivalent of clicking a control with the mouse
		// (Control::MouseUp() -> Invoke()) - exec-file mode has no real
		// mouse events, so this is the only way to exercise
		// GUI::ControlInvoked() (and whatever it routes to) headlessly.
		const ShellCommandParameters params = ParseParameters(argv);
		std::string chuNameString = params.at(0).value.string;
		uint16 windowId = (uint16)params.at(1).value.integer;
		// "-" (or empty) means the primary GUI resource (e.g. GUIW, the
		// HUD) rather than an aux screen - GetWindow() is the one that
		// looks there (GetAuxWindow() deliberately only checks aux
		// screens, since their plain numeric ids can collide across
		// different CHU files).
		Window* window = (chuNameString.empty() || chuNameString == "-")
			? GUI::Get()->GetWindow(windowId)
			: GUI::Get()->GetAuxWindow(res_ref(chuNameString.c_str()), windowId);
		if (window == NULL) {
			std::cout << "Invoke-Control: window not found" << std::endl;
			return;
		}
		Control* control = window->GetControlByID(params.at(2).value.integer);
		if (control == NULL) {
			std::cout << "Invoke-Control: control not found" << std::endl;
			return;
		}
		control->Invoke();
		std::cout << "Invoke-Control: OK" << std::endl;
	}
};


// Headless equivalent of right-clicking a control (Control::RightMouseDown)
// - exec-file mode has no real mouse events.
class RightClickControlCommand : public ShellCommand {
public:
	RightClickControlCommand()
		: ShellCommand(
			"RightClick-Control",
			{
				{ PARAMETER_STRING, }, // CHU name ("-" for the primary GUI)
				{ PARAMETER_INT, },    // window id
				{ PARAMETER_INT, }     // control id
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string chuNameString = params.at(0).value.string;
		uint16 windowId = (uint16)params.at(1).value.integer;
		Window* window = (chuNameString.empty() || chuNameString == "-")
			? GUI::Get()->GetWindow(windowId)
			: GUI::Get()->GetAuxWindow(res_ref(chuNameString.c_str()), windowId);
		if (window == NULL) {
			std::cout << "RightClick-Control: window not found" << std::endl;
			return;
		}
		Control* control = window->GetControlByID(params.at(2).value.integer);
		if (control == NULL) {
			std::cout << "RightClick-Control: control not found" << std::endl;
			return;
		}
		IE::point point = { 0, 0 };
		control->RightMouseDown(point);
		std::cout << "RightClick-Control: OK" << std::endl;
	}
};


// Headless equivalent of a mouse press-drag-release, in screen pixels:
// press at (x1,y1), move to (x2,y2), release. Exercises the whole
// GUI/Window/Control mouse path including capture (e.g. a scrollbar
// thumb drag).
class MouseDragCommand : public ShellCommand {
public:
	MouseDragCommand()
		: ShellCommand("Mouse-Drag", { { PARAMETER_POINT, }, { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point from = params.at(0).value.point;
		IE::point to = params.at(1).value.point;
		GUI::Get()->MouseDown(from.x, from.y);
		GUI::Get()->MouseMoved(to.x, to.y);
		GUI::Get()->MouseUp(to.x, to.y);
		std::cout << "Mouse-Drag: (" << std::dec << from.x << "," << from.y
			<< ") -> (" << to.x << "," << to.y << ")" << std::endl;
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


// Splits "actor,rest" on the FIRST comma only, returning false if there's
// no comma at all. Real trigger/action call syntax (e.g.
// "StartTimer(1,30)") has commas of its own, so ShellCommand's normal
// ParseParameters() (which splits argv on every comma before assigning
// declared parameters) mangles anything but a single-parameter trigger/
// action - found the hard way debugging this exact thing (STARTTIMER's
// second parameter always came back 0: ParseParameters() had cut
// "StartTimer(1,30)" into "StartTimer(1" and "30)", silently discarding
// the second piece, feeding the parser a truncated string that happened
// to still "parse" without error).
static bool
_SplitOnFirstComma(const char* argv, std::string& first, std::string& rest)
{
	const char* comma = ::strchr(argv, ',');
	if (comma == NULL)
		return false;
	first.assign(argv, comma - argv);
	rest.assign(comma + 1);
	return true;
}


class EvaluateTriggerCommand : public ShellCommand {
public:
	EvaluateTriggerCommand()
		: ShellCommand("Evaluate-Trigger")
	{
	}
	virtual void operator()(const char* argv) {
		// No other way to check a trigger's result directly - every
		// other trigger-driven test path in this console goes through a
		// whole dialog/script re-evaluation. Same parsing entry point
		// DialogHandler::_AdvanceState() already uses for real DLG state
		// triggers (Parser::TriggerFromString() + Script::EvaluateTrigger()).
		// Usage: Evaluate-Trigger <actor>,<trigger text, e.g. TimerExpired(1)>
		std::string actorName, triggerText;
		if (!_SplitOnFirstComma(argv, actorName, triggerText)) {
			std::cout << "Evaluate-Trigger: expected <actor>,<trigger text>" << std::endl;
			return;
		}

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "Evaluate-Trigger: actor not found" << std::endl;
			return;
		}

		trigger_params* trigger = Parser::TriggerFromString(triggerText);
		if (trigger == NULL) {
			std::cout << "Evaluate-Trigger: failed to parse trigger" << std::endl;
			return;
		}

		int orTrigger = 0;
		bool result = Script::EvaluateTrigger(actor, trigger, orTrigger);
		std::cout << "Evaluate-Trigger: " << (result ? "true" : "false") << std::endl;
		delete trigger;
	}
};


class QueueActionCommand : public ShellCommand {
public:
	QueueActionCommand()
		: ShellCommand("Queue-Action")
	{
	}
	virtual void operator()(const char* argv) {
		// Run-Action's own fixed positional params (integer1/string1/
		// string2/where) don't cover every action's real signature - e.g.
		// StartTimer(I:ID*,I:Time*) needs a *second* integer, which
		// Run-Action has no slot for. This parses real action syntax
		// instead (Parser::ActionFromString(), same entry point
		// DialogHandler::_ExecuteTransition() uses for real DLG action
		// blocks) so any action can be queued regardless of its
		// parameter shape. See _SplitOnFirstComma() above for why this
		// takes the raw text instead of going through ParseParameters().
		// Usage: Queue-Action <actor>,<action text, e.g. StartTimer(1,30)>
		std::string actorName, actionText;
		if (!_SplitOnFirstComma(argv, actorName, actionText)) {
			std::cout << "Queue-Action: expected <actor>,<action text>" << std::endl;
			return;
		}

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "Queue-Action: actor not found" << std::endl;
			return;
		}

		action_params* action = Parser::ActionFromString(actionText);
		if (action == NULL) {
			std::cout << "Queue-Action: failed to parse action" << std::endl;
			return;
		}

		actor->AddAction(action);
		action->Release();
		std::cout << "Queue-Action: OK" << std::endl;
	}
};


class ToggleSearchMapCommand : public ShellCommand {
public:
	ToggleSearchMapCommand()
		: ShellCommand("Toggle-SearchMap")
	{
	}
	virtual void operator()(const char* argv) {
		// Headless equivalent of the SDLK_s handler - lets a Screenshot
		// (see ScreenshotCommand) visualize the search map overlay
		// without a real key event.
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL) {
			std::cout << "Toggle-SearchMap: no area room" << std::endl;
			return;
		}
		room->ToggleSearchMap();
		std::cout << "Toggle-SearchMap: OK" << std::endl;
	}
};


class CheckPassableCommand : public ShellCommand {
public:
	CheckPassableCommand()
		: ShellCommand(
			"Check-Passable",
			{
				{ PARAMETER_POINT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		// Direct SearchMap query, bypassing pathfinding/movement
		// entirely - useful to confirm a door (or anything else) is
		// actually flipping a specific cell's passability, without the
		// noise of a real walk attempt (area scripts re-evaluating every
		// WAIT-TICKS tick print plenty of unrelated log lines).
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point point = params.at(0).value.point;
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL || room->SearchMap() == NULL) {
			std::cout << "Check-Passable: no search map" << std::endl;
			return;
		}
		bool passable = room->SearchMap()->IsPointPassable(point.x, point.y);
		// std::dec: some earlier, unrelated print in this session's own
		// command chain leaves cout in hex mode - not this command's bug
		// to carry forward into its own output.
		std::cout << std::dec << "Check-Passable (" << point.x << "," << point.y << "): "
			<< (passable ? "passable" : "BLOCKED") << std::endl;
	}
};


// CheckLineOfSightCommand - direct AreaRoom::HasLineOfSight() query
// between two explicit points, same "bypass the noise of a real
// actor/trigger" rationale as CheckPassableCommand above.
class CheckLineOfSightCommand : public ShellCommand {
public:
	CheckLineOfSightCommand()
		: ShellCommand(
			"Check-LineOfSight",
			{
				{ PARAMETER_POINT, },
				{ PARAMETER_POINT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point from = params.at(0).value.point;
		IE::point to = params.at(1).value.point;
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL) {
			std::cout << "Check-LineOfSight: no current area" << std::endl;
			return;
		}
		bool visible = room->HasLineOfSight(from, to);
		std::cout << std::dec << "Check-LineOfSight (" << from.x << "," << from.y
			<< ")-(" << to.x << "," << to.y << "): "
			<< (visible ? "clear" : "BLOCKED") << std::endl;
	}
};


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
	console->AddCommand(new ToggleInventoryCommand());
	console->AddCommand(new ToggleRecordCommand());
	console->AddCommand(new SelectPartyCommand());
	console->AddCommand(new EvaluateTriggerCommand());
	console->AddCommand(new QueueActionCommand());
	console->AddCommand(new CheckPassableCommand());
	console->AddCommand(new CheckLineOfSightCommand());
	console->AddCommand(new ToggleSearchMapCommand());
	console->AddCommand(new ToggleSaveCommand());
	console->AddCommand(new ToggleLoadCommand());
	console->AddCommand(new ToggleJournalCommand());
	console->AddCommand(new InvokeControlCommand());
	console->AddCommand(new RightClickControlCommand());
	console->AddCommand(new MouseDragCommand());
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
	console->AddCommand(new StepTicksCommand());
}
