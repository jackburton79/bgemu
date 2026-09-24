/*
 * Commands.cpp
 *
 *  Created on: 07/ott/2012
 *      Author: stefano
 */


#include "Commands.h"

#include "AnimationFactory.h"
#include "AreaRoom.h"
#include "CharacterBuilder.h"
#include "Container.h"
#include "Store.h"
#include "Control.h"
#include "Core.h"
#include "CreResource.h"
#include "Dialog.h"
#include "Door.h"
#include "Button.h"
#include "GamResource.h"
#include "ActionBar.h"
#include "Game.h"
#include "StartingParty.h"
#include "GameJournal.h"
#include "SavedGame.h"
#include "GameConsole.h"
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "MemoryStream.h"
#include "DLGResource.h"
#include "Parsing.h"
#include "InventoryScreen.h"
#include "JournalScreen.h"
#include "LootWindow.h"
#include "Party.h"
#include "Region.h"
#include "PlaylistStream.h"
#include "MusPlaylist.h"
#include "SoundEngine.h"
#include "GameFiles.h"
#include "ACMStream.h"
#include "TLKResource.h"
#include "WAVResource.h"
#include "RecordScreen.h"
#include "ScreenManager.h"
#include "StoreScreen.h"
#include "ResManager.h"
#include "Script.h"
#include "SearchMap.h"
#include "ShellCommand.h"
#include "Window.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include <SDL.h>

// Resolves the current room as an AreaRoom*, printing a message and
// returning NULL if it isn't one (e.g. the worldmap) - shared by every
// command below that assumes an AreaRoom. A blind C-style cast here used
// to read garbage/out-of-bounds memory once CurrentRoom() pointed at a
// WorldMap instead (a WorldMap doesn't share AreaRoom's fActors layout) -
// found via a real heap-buffer-overflow crash in GetObject()/GetActorsList()
// right after a wilderness map-edge exit loaded the worldmap.
static AreaRoom*
CurrentAreaRoom()
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room == NULL)
		std::cout << "command not available: current room is not an area." << std::endl;
	return room;
}


class ListObjectsCommand : public ShellCommand {
public:
	ListObjectsCommand()
		: ShellCommand("List-Objects")
	{
	}
	virtual ~ListObjectsCommand() {};
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		ActorsList objects;
		ActorsList::iterator i;
		room->GetActorsList(objects);
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
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		for (Container* container : room->Containers()) {
			IE::rect frame = container->Frame();
			std::cout << container->Name() << " (" << std::dec
				<< (frame.x_min + frame.x_max) / 2 << ","
				<< (frame.y_min + frame.y_max) / 2 << "):";
			for (const IE::item& item : container->ContainerItems())
				std::cout << " " << item.name.CString()
					<< "x" << (item.quantity1 > 0 ? item.quantity1 : 1);
			std::cout << std::endl;
		}
	}
};


// ListDoorsCommand - dumps door names/geometry/state in the current
// area, for diagnosing WED/ARE door mismatches (see AreaRoom::
// _InitDoors()'s own comment on AR0900-style malformed areas).
class ListDoorsCommand : public ShellCommand {
public:
	ListDoorsCommand()
		: ShellCommand("List-Doors")
	{
	}
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		for (Door* door : room->Doors()) {
			IE::rect openBox = door->OpenBox();
			IE::rect closedBox = door->ClosedBox();
			std::cout << door->Name() << " short=" << door->ShortName().CString()
				<< " opened=" << door->Opened()
				<< " openBox=(" << std::dec << openBox.x_min << "," << openBox.y_min
				<< ")-(" << openBox.x_max << "," << openBox.y_max << ")"
				<< " closedBox=(" << closedBox.x_min << "," << closedBox.y_min
				<< ")-(" << closedBox.x_max << "," << closedBox.y_max << ")"
				<< " tiles=" << door->fTilesOpen.size()
				<< std::endl;
		}
	}
};


// ListRegionsCommand - dumps the current area's regions (name, type, frame
// and, for a travel region, where it leads), to know where to send an actor.
class ListRegionsCommand : public ShellCommand {
public:
	ListRegionsCommand()
		: ShellCommand("List-Regions")
	{
	}
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		for (Region* region : room->Regions()) {
			IE::rect frame = region->Frame();
			std::cout << std::dec << region->Name() << " type=" << region->Type()
				<< " frame=(" << frame.x_min << "," << frame.y_min << ")-("
				<< frame.x_max << "," << frame.y_max << ")";
			if (region->Type() == IE::REGION_TYPE_TRAVEL) {
				std::cout << " -> " << region->DestinationArea().CString()
					<< " " << region->DestinationEntrance();
			}
			std::cout << std::endl;
		}
	}
};


// ListGroundCommand - dumps the loose item piles on the current area's
// floor (position + contents), for ground-item drop/pickup tests.
class ListGroundCommand : public ShellCommand {
public:
	ListGroundCommand() : ShellCommand("List-Ground") {}
	virtual void operator()(const char* argv) {
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL)
			return;
		const auto& piles = room->GroundPiles();
		std::cout << piles.size() << " ground pile(s)" << std::endl;
		for (size_t i = 0; i < piles.size(); i++) {
			std::cout << "  [" << i << "] (" << std::dec << piles[i].position.x
				<< "," << piles[i].position.y << "):";
			for (const IE::item& item : piles[i].items)
				std::cout << " " << item.name.CString()
					<< "x" << (item.quantity1 > 0 ? item.quantity1 : 1);
			std::cout << std::endl;
		}
	}
};


// PickUpGroundCommand - the selected party member auto-loots the ground
// pile at the given index (headless equivalent of clicking it).
class PickUpGroundCommand : public ShellCommand {
public:
	PickUpGroundCommand()
		: ShellCommand("Pickup-Ground", { { PARAMETER_INT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL)
			return;
		room->PickUpGroundPile((size_t)params.at(0).value.integer,
								room->SelectedActor());
	}
};


// Character-creation console commands (roadmap Fase 47 / A). Headless
// only for now - the GUICG* screens are a later sub-phase.
static int
_AbilityIndex(const std::string& name)
{
	for (int i = 0; i < CharacterBuilder::kNumAbilities; i++) {
		if (strcasecmp(CharacterBuilder::AbilityName(i), name.c_str()) == 0)
			return i;
	}
	return -1;
}

class CharNewCommand : public ShellCommand {
public:
	CharNewCommand() : ShellCommand("Char-New") {}
	virtual void operator()(const char* argv) {
		Game::Get()->Starting().Builder().Reset();
		std::cout << "new character" << std::endl;
	}
};

class CharSetCommand : public ShellCommand {
public:
	CharSetCommand()
		: ShellCommand("Char-Set", { { PARAMETER_STRING, }, { PARAMETER_STRING, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string field = params.at(0).value.string;
		std::string value = params.at(1).value.string;
		if (value == "-")
			value.clear();
		CharacterBuilder& b = Game::Get()->Starting().Builder();
		bool ok = false;
		if (strcasecmp(field.c_str(), "gender") == 0)      ok = b.SetGender(value);
		else if (strcasecmp(field.c_str(), "race") == 0)   ok = b.SetRace(value);
		else if (strcasecmp(field.c_str(), "class") == 0)  ok = b.SetClass(value);
		else if (strcasecmp(field.c_str(), "kit") == 0)    ok = b.SetKit(value);
		else if (strcasecmp(field.c_str(), "alignment") == 0) ok = b.SetAlignment(value);
		else { std::cout << "unknown field: " << field << std::endl; return; }
		std::cout << field << " = " << value << (ok ? " : ok" : " : REJECTED") << std::endl;
	}
};

class CharRollCommand : public ShellCommand {
public:
	CharRollCommand() : ShellCommand("Char-Roll") {}
	virtual void operator()(const char* argv) {
		int total = Game::Get()->Starting().Builder().RollAbilities();
		if (total == 0)
			std::cout << "roll failed (set race + class first, or impossible combo)" << std::endl;
		Game::Get()->Starting().Builder().Print();
	}
};

class CharAbilityCommand : public ShellCommand {
public:
	CharAbilityCommand()
		: ShellCommand("Char-Ability", { { PARAMETER_STRING, }, { PARAMETER_INT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		int idx = _AbilityIndex(params.at(0).value.string);
		if (idx < 0) { std::cout << "unknown ability" << std::endl; return; }
		bool ok = Game::Get()->Starting().Builder().SetAbility(idx, params.at(1).value.integer);
		std::cout << params.at(0).value.string << " = " << params.at(1).value.integer
				<< (ok ? " : ok" : " : REJECTED (out of legal range)") << std::endl;
	}
};

class CharPrintCommand : public ShellCommand {
public:
	CharPrintCommand() : ShellCommand("Char-Print") {}
	virtual void operator()(const char* argv) {
		Game::Get()->Starting().Builder().Print();
	}
};

// Char-Build [path] - serialize the builder to a CRE v1 blob, optionally
// write it to `path`, then re-parse it and print the key fields back
// (round-trip check).
class CharBuildCommand : public ShellCommand {
public:
	CharBuildCommand() : ShellCommand("Char-Build", { { PARAMETER_STRING, } }) {}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string path = params.at(0).value.string;

		std::vector<uint8> data;
		if (!Game::Get()->Starting().Builder().BuildCREData(data)) {
			std::cout << "Char-Build: character not complete" << std::endl;
			return;
		}
		std::cout << "Char-Build: " << data.size() << " bytes" << std::endl;

		if (!path.empty() && path != "-") {
			std::ofstream file(path, std::ios::binary);
			file.write((const char*)data.data(), data.size());
			std::cout << "  written to " << path << std::endl;
		}

		MemoryStream stream(data.data(), data.size(), false);
		CREResource* cre = new CREResource("PLAYER1");
		cre->Acquire(); // resources start at refcount 0
		if (cre->Load(&stream, 0, data.size())) {
			cre->Init();
			BaseAttributes attr;
			cre->GetAttributes(attr);
			std::cout << "  round-trip: race=" << IDTable::RaceAt(cre->Race())
				<< " class=" << IDTable::ClassAt(cre->Class())
				<< " align=" << IDTable::AlignmentAt(cre->Alignment())
				<< " animID=0x" << std::hex << cre->AnimationID() << std::dec
				<< " maxHP=" << cre->MaxHitPoints() << std::endl;
			std::cout << "  STR " << (int)attr.strength << " DEX " << (int)attr.dexterity
				<< " CON " << (int)attr.constitution << " INT " << (int)attr.intelligence
				<< " WIS " << (int)attr.wisdom << " CHR " << (int)attr.charisma << std::endl;
		} else {
			std::cout << "  round-trip FAILED to parse" << std::endl;
		}
		gResManager->ReleaseResource(cre);
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
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		Object* object = room->GetObject(name.c_str());

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
// GameJournal's header comment) so ADDJOURNALENTRY/
// ERASEJOURNALENTRY/SETQUESTDONE are testable headlessly.
static bool _SplitOnFirstComma(const char* argv, std::string& first, std::string& rest);


class PrintJournalCommand : public ShellCommand {
public:
	PrintJournalCommand()
		: ShellCommand("Print-Journal")
	{
	}
	virtual void operator()(const char* argv) {
		for (const journal_entry& entry : Game::Get()->Journal().Entries())
			std::cout << std::dec << entry.strref << " [section " << (int)entry.section << ", group "
				<< (int)entry.group << ", chapter " << (int)entry.chapter << "]: "
				<< IDTable::GetDialog(entry.strref) << std::endl;
	}
};


// Assert-JournalSection <strref>,<section> - which journal section (1
// quest, 2 completed, 4 info, 0 user note) an entry is in; -1 if it isn't
// in the journal at all.
class AssertJournalSectionCommand : public ShellCommand {
public:
	AssertJournalSectionCommand()
		: ShellCommand("Assert-JournalSection")
	{
	}
	virtual void operator()(const char* argv) {
		std::string strrefText, expectedText;
		if (!_SplitOnFirstComma(argv, strrefText, expectedText)) {
			std::cout << "ASSERT FAIL: expected <strref>,<section>" << std::endl;
			return;
		}
		uint32 strref = ::strtoul(strrefText.c_str(), NULL, 0);
		int expected = (int)::strtol(expectedText.c_str(), NULL, 0);
		int section = -1;
		for (const journal_entry& entry : Game::Get()->Journal().Entries()) {
			if (entry.strref == strref)
				section = entry.section;
		}
		if (section == expected) {
			std::cout << std::dec << "ASSERT OK: JournalSection(" << strref << ") == " << section
				<< std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: JournalSection(" << strref << ") - expected "
				<< expected << ", got " << section << std::endl;
		}
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


// Show-Character <partyIndex> - selects which party member subsequent
// per-character screens (Inventory/Record/Spellbook) show, headless
// equivalent of clicking a portrait.
class ShowCharacterCommand : public ShellCommand {
public:
	ShowCharacterCommand()
		: ShellCommand("Show-Character", { { PARAMETER_INT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		Game::Get()->ShowCharacter((uint16)params.at(0).value.integer);
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
		Game::Get()->Screens().Toggle<InventoryScreen>();
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
		Game::Get()->Screens().Toggle<RecordScreen>();
		std::cout << "Toggle-Record: OK" << std::endl;
	}
};


class ToggleHUDCommand : public ShellCommand {
public:
	ToggleHUDCommand()
		: ShellCommand("Toggle-HUD")
	{
	}
	virtual void operator()(const char* argv) {
		// Test-only equivalent of the SDLK_TAB handler.
		Game::Get()->ToggleHUD();
		std::cout << "Toggle-HUD: "
			<< (GUI::Get()->IsHUDHidden() ? "hidden" : "shown") << std::endl;
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
		Game::Get()->Screens().Toggle("GUISAVE");
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
		Game::Get()->Screens().Toggle("GUILOAD");
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
		Game::Get()->Screens().Toggle<JournalScreen>();
		std::cout << "Toggle-Journal: OK" << std::endl;
	}
};

class ToggleArcaneSpellbookCommand : public ShellCommand {
public:
	ToggleArcaneSpellbookCommand() : ShellCommand("Toggle-SpellbookArcane") {}
	virtual void operator()(const char* argv) {
		Game::Get()->Screens().Toggle("GUIMG");
		std::cout << "Toggle-SpellbookArcane: OK" << std::endl;
	}
};


class ToggleDivineSpellbookCommand : public ShellCommand {
public:
	ToggleDivineSpellbookCommand() : ShellCommand("Toggle-SpellbookDivine") {}
	virtual void operator()(const char* argv) {
		Game::Get()->Screens().Toggle("GUIPR");
		std::cout << "Toggle-SpellbookDivine: OK" << std::endl;
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
		GUI::Get()->MouseMoved(from.x, from.y);
		GUI::Get()->MouseDown(from.x, from.y);
		GUI::Get()->MouseMoved(to.x, to.y);
		GUI::Get()->MouseUp(to.x, to.y);
		std::cout << "Mouse-Drag: (" << std::dec << from.x << "," << from.y
			<< ") -> (" << to.x << "," << to.y << ")" << std::endl;
	}
};


// Headless mouse click at a screen pixel: hover there, press, release.
class MouseClickCommand : public ShellCommand {
public:
	MouseClickCommand()
		: ShellCommand("Mouse-Click", { { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point p = params.at(0).value.point;
		GUI::Get()->MouseMoved(p.x, p.y);
		GUI::Get()->MouseDown(p.x, p.y);
		GUI::Get()->MouseUp(p.x, p.y);
		std::cout << "Mouse-Click: (" << std::dec << p.x << "," << p.y << ")" << std::endl;
	}
};


// Hover only, no press/release - for testing hover-only feedback (a
// tooltip, a highlighted TextArea line) without also triggering
// whatever a real click at that point would do.
class MouseMoveCommand : public ShellCommand {
public:
	MouseMoveCommand()
		: ShellCommand("Mouse-Move", { { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point p = params.at(0).value.point;
		GUI::Get()->MouseMoved(p.x, p.y);
		std::cout << "Mouse-Move: (" << std::dec << p.x << "," << p.y << ")" << std::endl;
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
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		object = room->GetObject(name.c_str());

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
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		Object* object = room->GetObject(name.c_str());

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
// "-" (or empty) means "the current party leader" (Party::ActorAt(0))
// instead of a named room lookup - lets game-agnostic test scripts
// (tests/*.txt) reference some valid actor without hardcoding a CRE
// resref that only exists in one game's default starting party (BG1's
// is AJANTI, BG2's is ANOMEN10).
static Actor*
FindActor(const std::string& name)
{
	if (name.empty() || name == "-") {
		Party* party = Game::Get()->Party();
		Actor* actor = party != NULL ? party->ActorAt(0) : NULL;
		if (actor == NULL)
			std::cout << "no party leader (empty party?)." << std::endl;
		return actor;
	}

	AreaRoom* room = CurrentAreaRoom();
	if (room == NULL)
		return NULL;
	Object* object = room->GetObject(name.c_str());
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


// Assert-Trigger <actor>,<true|false>,<trigger text> - self-checking
// sibling of Evaluate-Trigger for regression test scripts (tests/*.txt,
// run headless via --exec-file): prints "ASSERT OK"/"ASSERT FAIL" instead
// of the bare result, so a test run is verified by grepping its output
// for "ASSERT FAIL" rather than eyeballing every line.
class AssertTriggerCommand : public ShellCommand {
public:
	AssertTriggerCommand()
		: ShellCommand("Assert-Trigger")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, rest;
		if (!_SplitOnFirstComma(argv, actorName, rest)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>,<trigger text>" << std::endl;
			return;
		}
		std::string expectedText, triggerText;
		if (!_SplitOnFirstComma(rest.c_str(), expectedText, triggerText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>,<trigger text>" << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: actor not found: " << triggerText << std::endl;
			return;
		}

		trigger_params* trigger = Parser::TriggerFromString(triggerText);
		if (trigger == NULL) {
			std::cout << "ASSERT FAIL: failed to parse trigger: " << triggerText << std::endl;
			return;
		}

		int orTrigger = 0;
		bool result = Script::EvaluateTrigger(actor, trigger, orTrigger);
		delete trigger;

		if (result == expected) {
			std::cout << "ASSERT OK: " << triggerText << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << triggerText << " - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (result ? "true" : "false") << std::endl;
		}
	}
};


// Assert-Triggers <actor>,<true|false>,<trigger1>;<trigger2>;... -
// Assert-Trigger's sibling for a whole AND/OR(N) trigger list (see
// Evaluate-Triggers's own comment).
class AssertTriggersCommand : public ShellCommand {
public:
	AssertTriggersCommand()
		: ShellCommand("Assert-Triggers")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, rest;
		if (!_SplitOnFirstComma(argv, actorName, rest)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>,<trigger1>;<trigger2>;..." << std::endl;
			return;
		}
		std::string expectedText, triggerText;
		if (!_SplitOnFirstComma(rest.c_str(), expectedText, triggerText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>,<trigger1>;<trigger2>;..." << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: actor not found: " << triggerText << std::endl;
			return;
		}

		// Keep the original ";"-joined text for display - the parser
		// needs "\n"-joined instead (see Evaluate-Triggers's own comment).
		std::string displayText = triggerText;
		std::replace(triggerText.begin(), triggerText.end(), ';', '\n');
		std::vector<trigger_params*> triggers = Parser::TriggersFromString(triggerText);
		if (triggers.empty()) {
			std::cout << "ASSERT FAIL: failed to parse any trigger: " << displayText << std::endl;
			return;
		}

		bool result = Script::EvaluateTriggerList(actor, triggers);
		for (trigger_params* t : triggers)
			delete t;

		if (result == expected) {
			std::cout << "ASSERT OK: " << displayText << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << displayText << " - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (result ? "true" : "false") << std::endl;
		}
	}
};


class EvaluateTriggersCommand : public ShellCommand {
public:
	EvaluateTriggersCommand()
		: ShellCommand("Evaluate-Triggers")
	{
	}
	virtual void operator()(const char* argv) {
		// Like Evaluate-Trigger, but for a whole trigger list (AND/OR(N)
		// sequence, e.g. a DLG state's trigger text) via Script::
		// EvaluateTriggerList() - the one thing Evaluate-Trigger can't
		// exercise, since it only ever parses/evaluates a single trigger.
		// Usage: Evaluate-Triggers <actor>,<trigger1>;<trigger2>;...
		std::string actorName, triggerText;
		if (!_SplitOnFirstComma(argv, actorName, triggerText)) {
			std::cout << "Evaluate-Triggers: expected <actor>,<trigger1>;<trigger2>;..." << std::endl;
			return;
		}

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "Evaluate-Triggers: actor not found" << std::endl;
			return;
		}

		std::replace(triggerText.begin(), triggerText.end(), ';', '\n');
		std::vector<trigger_params*> triggers = Parser::TriggersFromString(triggerText);
		if (triggers.empty()) {
			std::cout << "Evaluate-Triggers: failed to parse any trigger" << std::endl;
			return;
		}

		bool result = Script::EvaluateTriggerList(actor, triggers);
		std::cout << "Evaluate-Triggers: " << (result ? "true" : "false") << std::endl;
		for (trigger_params* t : triggers)
			delete t;
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


// Direct SearchMap::IsWorldmapExit() query, same spirit as
// Check-Passable above - a wilderness map's own "open the worldmap
// here" edge cells (search-map value 14, appendices/search.htm).
class CheckWorldmapExitCommand : public ShellCommand {
public:
	CheckWorldmapExitCommand()
		: ShellCommand("Check-WorldmapExit", { { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point point = params.at(0).value.point;
		AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
		if (room == NULL || room->SearchMap() == NULL) {
			std::cout << "Check-WorldmapExit: no search map" << std::endl;
			return;
		}
		bool isExit = room->SearchMap()->IsWorldmapExit(point.x, point.y);
		std::cout << std::dec << "Check-WorldmapExit (" << point.x << "," << point.y << "): "
			<< (isExit ? "YES" : "no") << std::endl;
	}
};


// Assert-AreaMapVisible <areaName>,<true|false> - same self-checking
// spirit as Assert-Trigger, for Game::AreaMapVisibleOverride() (a
// script's RevealAreaOnMap()/HideAreaOnMap(), or WorldMap's own leave-
// through-an-edge reveal - see WorldMap::_RevealAdjacentAreas()) since
// nothing exposes that as a trigger. Fails if the area was never
// overridden at all - the area's own file-authored bit alone doesn't
// prove either RevealAreaOnMap or an edge-reveal actually ran.
class AssertAreaMapVisibleCommand : public ShellCommand {
public:
	AssertAreaMapVisibleCommand()
		: ShellCommand("Assert-AreaMapVisible")
	{
	}
	virtual void operator()(const char* argv) {
		std::string areaName, expectedText;
		if (!_SplitOnFirstComma(argv, areaName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <areaName>,<true|false>" << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		bool visible;
		if (!Game::Get()->AreaMapVisibleOverride(areaName, &visible)) {
			std::cout << "ASSERT FAIL: AreaMapVisible(" << areaName
				<< ") - no override set" << std::endl;
			return;
		}
		if (visible == expected) {
			std::cout << "ASSERT OK: AreaMapVisible(" << areaName << ")" << std::endl;
		} else {
			std::cout << "ASSERT FAIL: AreaMapVisible(" << areaName << ") - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (visible ? "true" : "false") << std::endl;
		}
	}
};


// Assert-DoorOpened <doorName>,<true|false> - same self-checking spirit
// as Assert-Trigger, for a door's open/closed state: real IESDP's
// Open(O:Door*) trigger (TRIGGER.IDS 82, "OPENED") isn't implemented in
// this engine's Triggers.cpp yet, so there's no trigger round-trip to
// assert through. Used to verify area-checkpoint persistence (see
// AreaRoom::WriteCheckpoint()) across a Save-Game/Load-Game round trip.
class AssertDoorOpenedCommand : public ShellCommand {
public:
	AssertDoorOpenedCommand()
		: ShellCommand("Assert-DoorOpened")
	{
	}
	virtual void operator()(const char* argv) {
		std::string doorName, expectedText;
		if (!_SplitOnFirstComma(argv, doorName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <doorName>,<true|false>" << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL) {
			std::cout << "ASSERT FAIL: DoorOpened(" << doorName << ") - no current area"
				<< std::endl;
			return;
		}
		Door* door = dynamic_cast<Door*>(room->GetObject(doorName.c_str()));
		if (door == NULL) {
			std::cout << "ASSERT FAIL: DoorOpened(" << doorName << ") - door not found"
				<< std::endl;
			return;
		}
		bool opened = door->Opened();
		if (opened == expected) {
			std::cout << "ASSERT OK: DoorOpened(" << doorName << ")" << std::endl;
		} else {
			std::cout << "ASSERT FAIL: DoorOpened(" << doorName << ") - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (opened ? "true" : "false") << std::endl;
		}
	}
};


// Assert-Position <actor>,<x>,<y> - same self-checking spirit as
// Assert-DoorOpened, for an actor's exact position: no trigger exposes
// this either (Range(O:Object*,I:Distance*) only compares against
// another object, not an absolute point). Used to verify a party
// member's position survives a Save-Game/Load-Game round trip instead
// of being reset to wherever the loaded area's own entrance point is
// (see Game::Load()'s own comment on why that used to happen).
class AssertPositionCommand : public ShellCommand {
public:
	AssertPositionCommand()
		: ShellCommand(
			"Assert-Position",
			{
				{ PARAMETER_STRING, },
				{ PARAMETER_POINT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::string actorName = params.at(0).value.string;
		IE::point expected = params.at(1).value.point;

		Actor* actor = FindActor(actorName);
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: Position(" << actorName << ") - actor not found"
				<< std::endl;
			return;
		}
		IE::point position = actor->Position();
		if (position.x == expected.x && position.y == expected.y) {
			std::cout << "ASSERT OK: Position(" << actorName << ")" << std::endl;
		} else {
			// std::dec: an earlier command elsewhere may have left cout in
			// hex mode (it's a sticky stream flag) - always print this in
			// decimal regardless, matching what the caller passed in.
			std::cout << std::dec << "ASSERT FAIL: Position(" << actorName << ") - expected ("
				<< expected.x << "," << expected.y << "), got ("
				<< position.x << "," << position.y << ")" << std::endl;
		}
	}
};


// Assert-JournalHasEntry <strref>,<true|false> - same self-checking
// spirit as Assert-DoorOpened, for GameJournal::Strrefs(): no trigger
// exposes journal content, so this is the only way to assert on it
// (used to verify GamResource's journal round trip across Save-Game/
// Load-Game).
class AssertJournalHasEntryCommand : public ShellCommand {
public:
	AssertJournalHasEntryCommand()
		: ShellCommand("Assert-JournalHasEntry")
	{
	}
	virtual void operator()(const char* argv) {
		std::string strrefText, expectedText;
		if (!_SplitOnFirstComma(argv, strrefText, expectedText)) {
			std::cout << "ASSERT FAIL: expected <strref>,<true|false>" << std::endl;
			return;
		}
		uint32 strref = ::strtoul(strrefText.c_str(), NULL, 0);
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		const std::vector<uint32>& entries = Game::Get()->Journal().Strrefs();
		bool found = std::find(entries.begin(), entries.end(), strref) != entries.end();
		if (found == expected) {
			std::cout << std::dec << "ASSERT OK: JournalHasEntry(" << strref << ")" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: JournalHasEntry(" << strref << ") - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (found ? "true" : "false") << std::endl;
		}
	}
};


// Assert-ContainerHasItem <container name>,<item resref>,<true|false> -
// self-checking view of a Container's contents, for the loot window's
// regression test (nothing else exposes what a container still holds).
class AssertContainerHasItemCommand : public ShellCommand {
public:
	AssertContainerHasItemCommand()
		: ShellCommand("Assert-ContainerHasItem")
	{
	}
	virtual void operator()(const char* argv) {
		std::string containerName, rest, itemName, expectedText;
		if (!_SplitOnFirstComma(argv, containerName, rest)
				|| !_SplitOnFirstComma(rest.c_str(), itemName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <container>,<item>,<true|false>"
				<< std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;

		AreaRoom* room = CurrentAreaRoom();
		Container* target = NULL;
		if (room != NULL) {
			for (Container* container : room->Containers()) {
				if (strcasecmp(container->Name(), containerName.c_str()) == 0) {
					target = container;
					break;
				}
			}
		}
		if (target == NULL) {
			std::cout << "ASSERT FAIL: no container named " << containerName << std::endl;
			return;
		}

		bool found = false;
		for (const IE::item& item : target->ContainerItems())
			found = found || strcasecmp(item.name.CString(), itemName.c_str()) == 0;
		if (found == expected) {
			std::cout << "ASSERT OK: " << containerName << " has " << itemName
				<< " == " << (expected ? "true" : "false") << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << containerName << " has " << itemName
				<< " - expected " << (expected ? "true" : "false") << ", got "
				<< (found ? "true" : "false") << std::endl;
		}
	}
};


// Assert-LootWindow <true|false> - whether the loot window is open.
class AssertLootWindowCommand : public ShellCommand {
public:
	AssertLootWindowCommand()
		: ShellCommand("Assert-LootWindow")
	{
	}
	virtual void operator()(const char* argv) {
		bool expected = strcasecmp(argv, "true") == 0;
		bool open = Game::Get()->Loot().IsOpen();
		if (open == expected) {
			std::cout << "ASSERT OK: LootWindow open == " << (expected ? "true" : "false")
				<< std::endl;
		} else {
			std::cout << "ASSERT FAIL: LootWindow open - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (open ? "true" : "false") << std::endl;
		}
	}
};


// Assert-DialogFile <actor>,<resref> - the dialog file the creature talks with.
class AssertDialogFileCommand : public ShellCommand {
public:
	AssertDialogFileCommand()
		: ShellCommand(
			"Assert-DialogFile",
			{
				{ PARAMETER_STRING, }, // actor
				{ PARAMETER_STRING, }  // expected dialog resource
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		const std::string expected = params.at(1).value.string;
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const std::string found = actor->CRE()->DialogFile().CString();
		if (strcasecmp(found.c_str(), expected.c_str()) == 0) {
			std::cout << "ASSERT OK: " << actorName << " dialog " << found << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << actorName << " dialog - expected " << expected
				<< ", got " << found << std::endl;
		}
	}
};


// Assert-DialogActive <true|false> - whether a conversation is going on.
class AssertDialogActiveCommand : public ShellCommand {
public:
	AssertDialogActiveCommand()
		: ShellCommand("Assert-DialogActive")
	{
	}
	virtual void operator()(const char* argv) {
		const bool expected = strcasecmp(argv, "true") == 0;
		const bool active = Game::Get()->InDialogMode();
		if (active == expected) {
			std::cout << "ASSERT OK: dialog active == " << (expected ? "true" : "false")
				<< std::endl;
		} else {
			std::cout << "ASSERT FAIL: dialog active - expected " << (expected ? "true" : "false")
				<< ", got " << (active ? "true" : "false") << std::endl;
		}
	}
};


// Assert-Paused <true|false> - whether the game is paused.
class AssertPausedCommand : public ShellCommand {
public:
	AssertPausedCommand()
		: ShellCommand("Assert-Paused")
	{
	}
	virtual void operator()(const char* argv) {
		const bool expected = strcasecmp(argv, "true") == 0;
		const bool paused = Core::Get()->IsPaused();
		if (paused == expected) {
			std::cout << "ASSERT OK: paused == " << (expected ? "true" : "false") << std::endl;
		} else {
			std::cout << "ASSERT FAIL: paused - expected " << (expected ? "true" : "false")
				<< ", got " << (paused ? "true" : "false") << std::endl;
		}
	}
};


// Assert-ScreenOpen <CHU> <true|false> - whether a screen ported to
// GameScreen (see screens/) is open.
class AssertScreenOpenCommand : public ShellCommand {
public:
	AssertScreenOpenCommand()
		: ShellCommand(
			"Assert-ScreenOpen",
			{
				{ PARAMETER_STRING, }, // CHU name (e.g. GUIREC)
				{ PARAMETER_STRING, }  // true or false
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string chuName = params.at(0).value.string;
		const bool expected = strcasecmp(params.at(1).value.string, "true") == 0;
		GameScreen* screen = Game::Get()->Screens().Find(res_ref(chuName.c_str()));
		if (screen == NULL) {
			std::cout << "ASSERT FAIL: no screen " << chuName << std::endl;
			return;
		}
		if (screen->IsOpen() == expected) {
			std::cout << "ASSERT OK: " << chuName << " open == "
				<< (expected ? "true" : "false") << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << chuName << " open - expected "
				<< (expected ? "true" : "false") << std::endl;
		}
	}
};


// Print-Store <resref> - a store's current stock (opened earlier this
// session): resref, packs in stock (-1 = infinite), pack size.
class PrintStoreCommand : public ShellCommand {
public:
	PrintStoreCommand()
		: ShellCommand("Print-Store")
	{
	}
	virtual void operator()(const char* argv) {
		Store* store = Game::Get()->Screens().Find<StoreScreen>()->LoadedStore(argv);
		if (store == NULL) {
			std::cout << "Print-Store: " << argv << " isn't loaded" << std::endl;
			return;
		}
		std::cout << "Store " << argv << " (type " << store->Type() << ", flags 0x"
			<< std::hex << store->Flags() << std::dec << "):" << std::endl;
		for (const store_entry& entry : store->Items()) {
			std::cout << "  " << entry.item.name.CString() << " x" << entry.amount
				<< " (pack " << entry.item.quantity1 << ")" << std::endl;
		}
	}
};


// Print-Gam <resref> - the party and out-of-party NPC tables of a GAM
// resource (e.g. BALDUR, a new game's starting state): CRE, area, position.
class PrintGamCommand : public ShellCommand {
public:
	PrintGamCommand()
		: ShellCommand("Print-Gam")
	{
	}
	virtual void operator()(const char* argv) {
		GamResource* gam = gResManager->GetGAM(res_ref(argv));
		if (gam == NULL) {
			std::cout << "Print-Gam: no GAM " << argv << std::endl;
			return;
		}
		std::cout << std::dec << "Party: " << gam->PartyMemberCount() << std::endl;
		for (uint32 i = 0; i < gam->PartyMemberCount(); i++)
			_Print(gam->PartyMemberAt(i));
		std::cout << "NPCs: " << gam->OutOfPartyCount() << std::endl;
		for (uint32 i = 0; i < gam->OutOfPartyCount(); i++)
			_Print(gam->OutOfPartyAt(i));
		gResManager->ReleaseResource(gam);
	}

private:
	static void _Print(const gam_party_member& member) {
		std::cout << "  " << member.creName.CString() << " "
			<< member.areaName.CString() << " " << member.position.x
			<< "," << member.position.y << " face " << member.orientation
			<< std::endl;
	}
};


// Assert-ActorCount <cre name>,<n> - exactly n creatures with that CRE name
// stand in the current area (catches a creature placed twice).
class AssertActorCountCommand : public ShellCommand {
public:
	AssertActorCountCommand()
		: ShellCommand("Assert-ActorCount")
	{
	}
	virtual void operator()(const char* argv) {
		std::string name, countText;
		AreaRoom* room = CurrentAreaRoom();
		if (!_SplitOnFirstComma(argv, name, countText) || room == NULL) {
			std::cout << "ASSERT FAIL: expected <cre name>,<n> in an area" << std::endl;
			return;
		}
		ActorsList actors;
		room->GetActorsList(actors);
		int count = 0;
		for (Actor* actor : actors) {
			if (strcasecmp(actor->Name(), name.c_str()) == 0)
				count++;
		}
		if (count == atoi(countText.c_str())) {
			std::cout << "ASSERT OK: ActorCount(" << name << ")" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: ActorCount(" << name << ") - expected "
				<< countText << ", got " << count << std::endl;
		}
	}
};


// Assert-PortraitCount <n> - the HUD's party portrait bar shows n portraits.
class AssertPortraitCountCommand : public ShellCommand {
public:
	AssertPortraitCountCommand()
		: ShellCommand("Assert-PortraitCount")
	{
	}
	virtual void operator()(const char* argv) {
		Window* window = GUI::Get()->GetWindow(GUI::WINDOW_PLAYER_SLOTS);
		int count = 0;
		for (uint32 i = 0; window != NULL && i < 6; i++) {
			Button* button = dynamic_cast<Button*>(window->GetControlByID(i));
			if (button != NULL && button->HasIcon())
				count++;
		}
		if (count == atoi(argv)) {
			std::cout << "ASSERT OK: PortraitCount" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: PortraitCount - expected " << argv
				<< ", got " << count << std::endl;
		}
	}
};


// Print-Dialog <resref> - a whole dialog: each state with its trigger and
// text, and under it every transition (player text, trigger, action, and
// where it leads: END, or a state of this or another dialog).
class PrintDialogCommand : public ShellCommand {
public:
	PrintDialogCommand()
		: ShellCommand("Print-Dialog")
	{
	}
	virtual void operator()(const char* argv) {
		DLGResource* dlg = gResManager->GetDLG(res_ref(argv));
		if (dlg == NULL) {
			std::cout << "Print-Dialog: no dialog " << argv << std::endl;
			return;
		}
		std::cout << std::dec;
		for (uint32 i = 0; i < dlg->CountStates(); i++) {
			dlg_state state = dlg->GetStateAt(i);
			std::cout << "State " << i;
			if (state.trigger != -1)
				std::cout << " [when " << _OneLine(dlg->GetStateTrigger(state.trigger)) << "]";
			std::cout << ": " << IDTable::GetDialog(state.text_ref);
			if (TLKEntry* entry = IDTable::GetTLKEntry(state.text_ref)) {
				if (entry->sound_ref.CString()[0] != '\0')
					std::cout << " <" << entry->sound_ref.CString() << ">";
				delete entry;
			}
			std::cout << std::endl;
			for (int32 t = 0; t < state.transitions_num; t++) {
				transition_entry transition = dlg->GetTransition(state.transition_first + t);
				std::cout << "  -> ";
				if (transition.HasTrigger())
					std::cout << "[if " << _OneLine(dlg->GetTransitionTrigger(transition.index_trigger)) << "] ";
				if (transition.HasPlayerText())
					std::cout << "\"" << IDTable::GetDialog(transition.text_player) << "\" ";
				if (transition.HasActions())
					std::cout << "{do " << _OneLine(dlg->GetAction(transition.index_action)) << "} ";
				if (transition.flags & DLG_TRANSITION_END)
					std::cout << "END";
				else
					std::cout << transition.resource_next_state.CString() << "#"
						<< transition.index_next_state;
				std::cout << std::endl;
			}
		}
		gResManager->ReleaseResource(dlg);
	}

private:
	static std::string _OneLine(std::string text) {
		std::replace(text.begin(), text.end(), '\n', ' ');
		return text;
	}
};


// Check-Dialogs [resref] - parses every state trigger, transition trigger
// and transition action of every DLG the game ships (or of just <resref>,
// printing each text before parsing it - what a conversation only does one
// state at a time, while playing) and reports how many texts were scanned:
// a trigger/action name the engine doesn't know shows up in the normal
// "GetTriggerID: value not found for"/"GetActionID: no action found" log
// lines, all at once instead of scattered through play.
class CheckDialogsCommand : public ShellCommand {
public:
	CheckDialogsCommand()
		: ShellCommand("Check-Dialogs")
	{
	}
	virtual void operator()(const char* argv) {
		std::vector<res_ref> names;
		const bool single = argv != NULL && argv[0] != '\0';
		if (single)
			names.push_back(res_ref(argv));
		else
			names = gResManager->ResourceNames(RES_DLG);

		uint32 dialogs = 0, texts = 0;
		for (const res_ref& name : names) {
			DLGResource* dlg = gResManager->GetDLG(name);
			if (dlg == NULL)
				continue;
			dialogs++;
			if (!single)
				std::cout << "Check-Dialogs: in " << name.CString() << std::endl;
			for (uint32 i = 0; i < dlg->CountStates(); i++) {
				dlg_state state = dlg->GetStateAt(i);
				if (state.trigger == -1)
					continue;
				std::string text = dlg->GetStateTrigger(state.trigger);
				_Show(single, name, "state trigger", text);
				_Free(Parser::TriggersFromString(text));
				texts++;
			}
			for (uint32 i = 0; i < dlg->CountTransitions(); i++) {
				transition_entry transition = dlg->GetTransition(i);
				if (transition.HasTrigger()) {
					std::string text = dlg->GetTransitionTrigger(transition.index_trigger);
					_Show(single, name, "transition trigger", text);
					_Free(Parser::TriggersFromString(text));
					texts++;
				}
				if (transition.HasActions()) {
					std::string text = dlg->GetAction(transition.index_action);
					_Show(single, name, "transition action", text);
					for (action_params* params : Parser::ActionsFromString(text))
						params->Release();
					texts++;
				}
			}
			gResManager->ReleaseResource(dlg);
		}
		std::cout << std::dec << "Check-Dialogs: " << dialogs << " dialogs, "
			<< texts << " trigger/action texts" << std::endl;
	}

private:
	static void _Show(bool single, const res_ref& name, const char* kind,
			const std::string& text) {
		if (single)
			std::cout << "Check-Dialogs: " << name.CString() << " " << kind
				<< ": [" << text << "]" << std::endl;
	}

	static void _Free(const std::vector<trigger_params*>& triggers) {
		for (trigger_params* trigger : triggers)
			delete trigger;
	}
};


// Assert-GamNPC <gam>,<cre>,<area>,<x>,<y> - the GAM resource lists <cre>
// as an out-of-party NPC standing in <area> at that point.
class AssertGamNPCCommand : public ShellCommand {
public:
	AssertGamNPCCommand()
		: ShellCommand("Assert-GamNPC")
	{
	}
	virtual void operator()(const char* argv) {
		std::vector<std::string> fields;
		std::stringstream stream(argv);
		for (std::string field; std::getline(stream, field, ',');)
			fields.push_back(field);
		if (fields.size() != 5) {
			std::cout << "ASSERT FAIL: expected <gam>,<cre>,<area>,<x>,<y>" << std::endl;
			return;
		}
		GamResource* gam = gResManager->GetGAM(res_ref(fields[0].c_str()));
		if (gam == NULL) {
			std::cout << "ASSERT FAIL: no GAM " << fields[0] << std::endl;
			return;
		}
		bool found = false;
		for (uint32 i = 0; i < gam->OutOfPartyCount() && !found; i++) {
			gam_party_member member = gam->OutOfPartyAt(i);
			found = strcasecmp(member.creName.CString(), fields[1].c_str()) == 0
				&& strcasecmp(member.areaName.CString(), fields[2].c_str()) == 0
				&& member.position.x == atoi(fields[3].c_str())
				&& member.position.y == atoi(fields[4].c_str());
		}
		gResManager->ReleaseResource(gam);
		std::cout << (found ? "ASSERT OK: " : "ASSERT FAIL: ") << argv << std::endl;
	}
};


// Assert-ItemIdentified <actor>,<item resref>,<true|false> - whether the
// first copy of an item in an actor's inventory counts as identified (its
// own flag, or an item that has no lore to identify - see
// Store::SlotFlags()).
class AssertItemIdentifiedCommand : public ShellCommand {
public:
	AssertItemIdentifiedCommand()
		: ShellCommand("Assert-ItemIdentified")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, rest, itemName, expectedText;
		if (!_SplitOnFirstComma(argv, actorName, rest)
				|| !_SplitOnFirstComma(rest.c_str(), itemName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<item>,<true|false>" << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
			IE::item item;
			if (!actor->CRE()->GetItemAtSlot(slot, item)
					|| strcasecmp(item.name.CString(), itemName.c_str()) != 0)
				continue;
			bool identified = (Store::SlotFlags(item) & STORE_ITEM_IDENTIFIED) != 0;
			if (identified == expected) {
				std::cout << "ASSERT OK: " << itemName << " identified == "
					<< (expected ? "true" : "false") << std::endl;
			} else {
				std::cout << "ASSERT FAIL: " << itemName << " identified - expected "
					<< (expected ? "true" : "false") << ", got "
					<< (identified ? "true" : "false") << std::endl;
			}
			return;
		}
		std::cout << "ASSERT FAIL: " << actorName << " has no " << itemName << std::endl;
	}
};


// Select-Weapon <actor>,<index> - puts weapon quickslot 0-3 in hand
// (-1 for fists), as the action bar's weapon buttons will.
class SelectWeaponCommand : public ShellCommand {
public:
	SelectWeaponCommand()
		: ShellCommand("Select-Weapon")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, indexText;
		if (!_SplitOnFirstComma(argv, actorName, indexText)) {
			std::cout << "expected <actor>,<index>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL)
			return;
		if (!actor->SelectWeapon(atoi(indexText.c_str())))
			std::cout << "no such weapon slot " << indexText << std::endl;
	}
};


// Select-Actor <name> - replaces the map selection with just this party
// member (headless equivalent of clicking their avatar or HUD portrait
// with no modifier).
class SelectActorCommand : public ShellCommand {
public:
	SelectActorCommand()
		: ShellCommand("Select-Actor")
	{
	}
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		Actor* actor = FindActor(argv);
		if (room != NULL && actor != NULL)
			room->SelectActor(actor);
	}
};


// Toggle-Selected <name> - adds/removes this party member from the map
// selection without touching anyone else's (headless equivalent of a
// shift-click on their avatar or HUD portrait).
class ToggleSelectedCommand : public ShellCommand {
public:
	ToggleSelectedCommand()
		: ShellCommand("Toggle-Selected")
	{
	}
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		Actor* actor = FindActor(argv);
		if (room != NULL && actor != NULL)
			room->ToggleSelected(actor);
	}
};


// Drag-Select <x1,y1>,<x2,y2>,<true|false> - headless equivalent of a
// rectangle drag-select on the map (both corners in screen/area coordinates
// - Click-Area's own convention), the trailing flag matching whether shift
// was held (adds to the current selection instead of replacing it). Skips
// the real mouse-drag gesture (MouseDown/MouseMoved/MouseUp with a real
// modifier key held), which headless testing can't drive - see
// AreaRoom::DragSelectAt().
class DragSelectCommand : public ShellCommand {
public:
	DragSelectCommand()
		: ShellCommand(
			"Drag-Select",
			{
				{ PARAMETER_POINT, },
				{ PARAMETER_POINT, },
				{ PARAMETER_STRING, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		bool additive = strcasecmp(params.at(2).value.string, "true") == 0;
		room->DragSelectAt(params.at(0).value.point, params.at(1).value.point, additive);
	}
};


// Assert-Selected <name>,<true|false> - whether this party member is
// currently part of the map selection.
class AssertSelectedCommand : public ShellCommand {
public:
	AssertSelectedCommand()
		: ShellCommand("Assert-Selected")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, expectedText;
		if (!_SplitOnFirstComma(argv, actorName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		bool expected = strcasecmp(expectedText.c_str(), "true") == 0;
		if (actor->IsSelected() == expected) {
			std::cout << "ASSERT OK: Selected(" << actorName << ")" << std::endl;
		} else {
			std::cout << "ASSERT FAIL: Selected(" << actorName << ") - expected "
				<< expectedText << ", got " << (actor->IsSelected() ? "true" : "false")
				<< std::endl;
		}
	}
};


// Assert-SelectedCount <n> - how many actors are currently in the map
// selection.
class AssertSelectedCountCommand : public ShellCommand {
public:
	AssertSelectedCountCommand()
		: ShellCommand("Assert-SelectedCount")
	{
	}
	virtual void operator()(const char* argv) {
		AreaRoom* room = CurrentAreaRoom();
		uint32 count = room != NULL ? room->CountSelectedActors() : 0;
		if (count == (uint32)atoi(argv)) {
			std::cout << "ASSERT OK: SelectedCount" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: SelectedCount - expected " << argv
				<< ", got " << count << std::endl;
		}
	}
};


// Assert-CustomColors <actor>,<true|false> - whether the creature's sprite is
// recolored from its CRE's color bytes (false: drawn with the BAM's own
// palette).
class AssertCustomColorsCommand : public ShellCommand {
public:
	AssertCustomColorsCommand()
		: ShellCommand("Assert-CustomColors")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, expectedText;
		if (!_SplitOnFirstComma(argv, actorName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<true|false>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const bool expected = strcasecmp(expectedText.c_str(), "true") == 0;
		const bool actual = AnimationFactory::UsesCustomColors(actor->CRE()->AnimationID());
		if (actual == expected)
			std::cout << "ASSERT OK: " << actorName << " custom colors == " << expectedText << std::endl;
		else
			std::cout << "ASSERT FAIL: " << actorName << " custom colors - expected "
				<< expectedText << std::endl;
	}
};


// Assert-ActorHasColors <actor> - the creature's sprite has a color set (an
// actor without one is drawn with the BAM's raw placeholder palette).
class AssertActorHasColorsCommand : public ShellCommand {
public:
	AssertActorHasColorsCommand()
		: ShellCommand("Assert-ActorHasColors")
	{
	}
	virtual void operator()(const char* argv) {
		Actor* actor = FindActor(argv);
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: no actor " << argv << std::endl;
			return;
		}
		if (actor->HasColors())
			std::cout << "ASSERT OK: " << argv << " has colors" << std::endl;
		else
			std::cout << "ASSERT FAIL: " << argv << " has no colors" << std::endl;
	}
};


// Assert-Paperdoll <actor>,<resource> - the paperdoll shown for the creature
// is this resource (and it exists).
class AssertPaperdollCommand : public ShellCommand {
public:
	AssertPaperdollCommand()
		: ShellCommand(
			"Assert-Paperdoll",
			{
				{ PARAMETER_STRING, }, // actor
				{ PARAMETER_STRING, }  // expected paperdoll resource
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		const std::string expected = params.at(1).value.string;
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const std::string name = actor->PaperdollName();
		if (name != expected) {
			std::cout << "ASSERT FAIL: " << actorName << " paperdoll - expected "
				<< expected << ", got " << name << std::endl;
		} else if (!gResManager->ResourceExists(name.c_str(),
				Core::Get()->Game() == game::GAME_BALDURSGATE ? RES_BAM : RES_PLT)) {
			std::cout << "ASSERT FAIL: " << actorName << " paperdoll " << name
				<< " does not exist" << std::endl;
		} else {
			std::cout << "ASSERT OK: " << actorName << " paperdoll " << name << std::endl;
		}
	}
};


// Music streaming (SoundEngine::PlayStream()): Play-Music-File <path> starts a
// loose ACM file (a path below the game's directory, case-insensitive, e.g.
// music/BC1/BC1A1.acm) at full volume, Stop-Music [<fadeMs>] ends it,
// Print-Music says what is playing and Assert-Music <true|false> checks that
// something plays. Wait-Audio <ms> waits real time (the audio thread runs on
// its own clock, not the game's ticks). Dump-Music-File <path>,<out.wav>
// decodes a track to a WAV file.
class PlayMusicFileCommand : public ShellCommand {
public:
	PlayMusicFileCommand()
		: ShellCommand("Play-Music-File")
	{
	}
	virtual void operator()(const char* argv) {
		const std::string path = FindGameFile(argv);
		ACMStream* stream = path.empty() ? NULL : ACMStream::Open(path);
		if (stream == NULL) {
			std::cout << "Play-Music-File: can't open " << argv << std::endl;
			return;
		}
		const uint32 duration = stream->DurationMs();
		const bool ok = SoundEngine::Get() != NULL
			&& SoundEngine::Get()->PlayStream(stream);
		std::cout << "Play-Music-File: " << (ok ? "OK" : "FAILED") << " (" << duration
			<< " ms)" << std::endl;
	}
};


class StopMusicCommand : public ShellCommand {
public:
	StopMusicCommand()
		: ShellCommand("Stop-Music")
	{
	}
	virtual void operator()(const char* argv) {
		if (SoundEngine::Get() != NULL)
			SoundEngine::Get()->StopStream((uint32)atoi(argv));
		std::cout << "Stop-Music: OK" << std::endl;
	}
};


class PrintMusicCommand : public ShellCommand {
public:
	PrintMusicCommand()
		: ShellCommand("Print-Music")
	{
	}
	virtual void operator()(const char* argv) {
		SoundEngine* engine = SoundEngine::Get();
		if (engine == NULL) {
			std::cout << "Print-Music: no sound engine" << std::endl;
			return;
		}
		std::cout << "Music: " << (engine->IsStreamPlaying() ? "playing" : "stopped")
			<< ", " << engine->StreamPositionMs() << " ms, volume "
			<< engine->StreamVolume() << std::endl;
	}
};


class AssertMusicCommand : public ShellCommand {
public:
	AssertMusicCommand()
		: ShellCommand("Assert-Music")
	{
	}
	virtual void operator()(const char* argv) {
		SoundEngine* engine = SoundEngine::Get();
		const bool playing = engine != NULL && engine->IsStreamPlaying();
		const bool expected = std::string(argv) == "true";
		if (playing == expected)
			std::cout << "ASSERT OK: music playing == " << playing << std::endl;
		else
			std::cout << "ASSERT FAIL: music playing is " << playing << ", expected "
				<< expected << std::endl;
	}
};


class AssertMusicPositionCommand : public ShellCommand {
public:
	AssertMusicPositionCommand()
		: ShellCommand("Assert-MusicPosition")
	{
	}
	// The stream has played at least this many milliseconds.
	virtual void operator()(const char* argv) {
		SoundEngine* engine = SoundEngine::Get();
		const uint32 position = engine != NULL ? engine->StreamPositionMs() : 0;
		if (position >= (uint32)atoi(argv))
			std::cout << "ASSERT OK: music position " << position << " ms" << std::endl;
		else
			std::cout << "ASSERT FAIL: music position " << position << " ms, expected at least "
				<< atoi(argv) << std::endl;
	}
};


class WaitAudioCommand : public ShellCommand {
public:
	WaitAudioCommand()
		: ShellCommand("Wait-Audio")
	{
	}
	virtual void operator()(const char* argv) {
		std::this_thread::sleep_for(std::chrono::milliseconds(atoi(argv)));
	}
};


class DumpMusicFileCommand : public ShellCommand {
public:
	DumpMusicFileCommand()
		: ShellCommand(
			"Dump-Music-File",
			{
				{ PARAMETER_STRING, },	// path below the game directory
				{ PARAMETER_STRING, }	// output path
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string path = FindGameFile(params.at(0).value.string);
		ACMStream* stream = path.empty() ? NULL : ACMStream::Open(path);
		if (stream == NULL) {
			std::cout << "Dump-Music-File: can't open " << params.at(0).value.string << std::endl;
			return;
		}
		std::vector<uint8> pcm;
		uint8 buffer[4096];
		for (size_t got; (got = stream->Read(buffer, sizeof(buffer))) > 0; )
			pcm.insert(pcm.end(), buffer, buffer + got);
		const uint16 channels = stream->Channels();
		const uint32 rate = stream->SampleRate();
		delete stream;

		std::ofstream file(params.at(1).value.string, std::ios::binary);
		auto put32 = [&] (uint32 v) { file.write(reinterpret_cast<const char*>(&v), 4); };
		auto put16 = [&] (uint16 v) { file.write(reinterpret_cast<const char*>(&v), 2); };
		file.write("RIFF", 4);
		put32(36 + (uint32)pcm.size());
		file.write("WAVEfmt ", 8);
		put32(16);
		put16(1);
		put16(channels);
		put32(rate);
		put32(rate * channels * 2);
		put16((uint16)(channels * 2));
		put16(16);
		file.write("data", 4);
		put32((uint32)pcm.size());
		file.write(reinterpret_cast<const char*>(pcm.data()), (std::streamsize)pcm.size());
		std::cout << std::dec << "Dump-Music-File: " << pcm.size() << " bytes, " << channels
			<< " channel(s), " << rate << " Hz" << std::endl;
	}
};


// Playlists (.mus files of music/): Play-Playlist <name.mus> streams one from
// its first entry (PlaylistStream), End-Playlist asks it to end (its current
// entry's interrupt track plays, then it stops), Print-Playlist <name.mus>
// lists its entries with their loop and interrupt tracks, Assert-MusicTrack
// <track> checks which track plays now, Assert-PlaylistNext <name.mus>,<track>,
// <next> checks the entry the playlist goes to after a track (a loop or the
// next line), Check-Music [<name.mus>] parses every playlist (or one) and
// reports the tracks that aren't in music/.
class PlayPlaylistCommand : public ShellCommand {
public:
	PlayPlaylistCommand()
		: ShellCommand("Play-Playlist")
	{
	}
	virtual void operator()(const char* argv) {
		MusPlaylist playlist;
		if (!MusPlaylist::Load(argv, playlist)) {
			std::cout << "Play-Playlist: can't read " << argv << std::endl;
			return;
		}
		PlaylistStream* stream = new PlaylistStream(playlist);
		if (!stream->Valid()) {
			delete stream;
			std::cout << "Play-Playlist: no track of " << argv << " plays" << std::endl;
			return;
		}
		const bool ok = SoundEngine::Get() != NULL && SoundEngine::Get()->PlayStream(stream);
		std::cout << "Play-Playlist: " << (ok ? "OK" : "FAILED") << std::endl;
	}
};


class EndPlaylistCommand : public ShellCommand {
public:
	EndPlaylistCommand()
		: ShellCommand("End-Playlist")
	{
	}
	virtual void operator()(const char* argv) {
		SoundEngine* engine = SoundEngine::Get();
		PlaylistStream* stream = engine != NULL
			? dynamic_cast<PlaylistStream*>(engine->Stream()) : NULL;
		if (stream == NULL) {
			std::cout << "End-Playlist: no playlist" << std::endl;
			return;
		}
		stream->RequestEnd();
		std::cout << "End-Playlist: OK" << std::endl;
	}
};


class PrintPlaylistCommand : public ShellCommand {
public:
	PrintPlaylistCommand()
		: ShellCommand("Print-Playlist")
	{
	}
	virtual void operator()(const char* argv) {
		MusPlaylist playlist;
		if (!MusPlaylist::Load(argv, playlist)) {
			std::cout << "Print-Playlist: can't read " << argv << std::endl;
			return;
		}
		std::cout << "Playlist " << argv << " (folder " << playlist.Folder() << ", "
			<< playlist.Count() << " entries):" << std::endl;
		for (size_t i = 0; i < playlist.Count(); i++) {
			const MusEntry& entry = playlist.At(i);
			std::cout << "  " << i << ": " << entry.track;
			if (!entry.folder.empty())
				std::cout << " (folder " << entry.folder << ")";
			if (!entry.loopTrack.empty())
				std::cout << " loop " << entry.loopFolder << (entry.loopFolder.empty() ? "" : " ")
					<< entry.loopTrack;
			if (!entry.end.empty())
				std::cout << " end " << entry.end;
			std::cout << " -> " << playlist.Next(i) << std::endl;
		}
	}
};


class AssertMusicTrackCommand : public ShellCommand {
public:
	AssertMusicTrackCommand()
		: ShellCommand("Assert-MusicTrack")
	{
	}
	virtual void operator()(const char* argv) {
		SoundEngine* engine = SoundEngine::Get();
		PlaylistStream* stream = engine != NULL
			? dynamic_cast<PlaylistStream*>(engine->Stream()) : NULL;
		const std::string track = stream != NULL && engine->IsStreamPlaying()
			? stream->CurrentTrack() : "";
		if (strcasecmp(track.c_str(), argv) == 0)
			std::cout << "ASSERT OK: music track " << track << std::endl;
		else
			std::cout << "ASSERT FAIL: music track \"" << track << "\", expected \""
				<< argv << "\"" << std::endl;
	}
};


class AssertPlaylistNextCommand : public ShellCommand {
public:
	AssertPlaylistNextCommand()
		: ShellCommand(
			"Assert-PlaylistNext",
			{
				{ PARAMETER_STRING, },	// playlist
				{ PARAMETER_STRING, },	// track
				{ PARAMETER_STRING, }	// expected next track
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		MusPlaylist playlist;
		if (!MusPlaylist::Load(params.at(0).value.string, playlist)) {
			std::cout << "ASSERT FAIL: can't read " << params.at(0).value.string << std::endl;
			return;
		}
		const int from = playlist.IndexOf(params.at(1).value.string);
		const int next = from >= 0 ? playlist.Next((size_t)from) : -1;
		const std::string nextTrack = next >= 0 ? playlist.At((size_t)next).track : "";
		if (strcasecmp(nextTrack.c_str(), params.at(2).value.string) == 0) {
			std::cout << "ASSERT OK: " << params.at(1).value.string << " -> " << nextTrack
				<< std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << params.at(1).value.string << " -> \"" << nextTrack
				<< "\", expected \"" << params.at(2).value.string << "\"" << std::endl;
		}
	}
};


class CheckMusicCommand : public ShellCommand {
public:
	CheckMusicCommand()
		: ShellCommand("Check-Music")
	{
	}
	virtual void operator()(const char* argv) {
		std::vector<std::string> names;
		if (argv != NULL && argv[0] != '\0') {
			names.push_back(argv);
		} else {
			const std::string directory = FindGameFile("music");
			std::error_code error;
			for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
				const std::string name = entry.path().filename().string();
				if (name.size() > 4 && strcasecmp(name.c_str() + name.size() - 4, ".mus") == 0)
					names.push_back(name);
			}
			std::sort(names.begin(), names.end());
		}

		int playlists = 0, entries = 0, missing = 0, unreadable = 0;
		for (const std::string& name : names) {
			MusPlaylist playlist;
			if (!MusPlaylist::Load(name, playlist)) {
				std::cout << "Check-Music: can't read " << name << std::endl;
				unreadable++;
				continue;
			}
			playlists++;
			for (size_t i = 0; i < playlist.Count(); i++) {
				entries++;
				std::vector<std::string> paths = { playlist.TrackPath(i) };
				if (!playlist.InterruptPath(i).empty())
					paths.push_back(playlist.InterruptPath(i));
				for (const std::string& path : paths) {
					if (FindGameFile(path).empty()) {
						std::cout << "Check-Music: " << name << " entry " << i << ": "
							<< path << " is missing" << std::endl;
						missing++;
					}
				}
			}
		}
		std::cout << std::dec << "Check-Music: " << playlists << " playlists, " << entries
			<< " entries, " << unreadable << " unreadable, " << missing << " problems"
			<< std::endl;
	}
};


// Play-Movie <name> plays a movie (an MVE resource) like STARTMOVIE.
// Assert-MovieSkip <name> plays it after an Escape key press was queued and
// asserts it ends at once (a movie lasts seconds): that a key skips a movie.
class PlayMovieCommand : public ShellCommand {
public:
	PlayMovieCommand()
		: ShellCommand("Play-Movie")
	{
	}
	virtual void operator()(const char* argv) {
		Core::Get()->PlayMovie(argv);
		std::cout << "Play-Movie: done" << std::endl;
	}
};


class AssertMovieSkipCommand : public ShellCommand {
public:
	AssertMovieSkipCommand()
		: ShellCommand("Assert-MovieSkip")
	{
	}
	virtual void operator()(const char* argv) {
		SDL_Event key;
		SDL_zero(key);
		key.type = SDL_KEYDOWN;
		key.key.keysym.sym = SDLK_ESCAPE;
		SDL_PushEvent(&key);

		const uint32 start = SDL_GetTicks();
		Core::Get()->PlayMovie(argv);
		const uint32 elapsed = SDL_GetTicks() - start;
		if (elapsed < 2000)
			std::cout << "ASSERT OK: movie " << argv << " skipped after " << elapsed << " ms"
				<< std::endl;
		else
			std::cout << "ASSERT FAIL: movie " << argv << " played for " << elapsed << " ms"
				<< std::endl;
	}
};


// Assert-CanLevelUp <actor>,<true|false> - whether the actor's experience
// allows a level above the current one (see Actor::CanLevelUp()).
class AssertCanLevelUpCommand : public ShellCommand {
public:
	AssertCanLevelUpCommand()
		: ShellCommand(
			"Assert-CanLevelUp",
			{
				{ PARAMETER_STRING, },	// actor
				{ PARAMETER_STRING, }	// expected: true or false
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		const bool expected = std::string(params.at(1).value.string) == "true";
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const bool can = actor->CanLevelUp();
		if (can == expected)
			std::cout << "ASSERT OK: " << actorName << " can level up == " << can << std::endl;
		else
			std::cout << "ASSERT FAIL: " << actorName << " can level up is " << can
				<< ", expected " << expected << std::endl;
	}
};


// Level-Up <actor> - the actor gains the levels its experience allows (what
// the Record screen's Level Up button does for the shown character).
class LevelUpCommand : public ShellCommand {
public:
	LevelUpCommand()
		: ShellCommand("Level-Up")
	{
	}
	virtual void operator()(const char* argv) {
		Actor* actor = FindActor(argv);
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "Level-Up: no actor " << argv << std::endl;
			return;
		}
		std::cout << "Level-Up: " << (actor->LevelUp() ? "OK" : "nothing to gain") << std::endl;
	}
};


// Assert-SpellSlots <actor>,<type>,<spellLevel>,<count> - how many spells of
// this level the actor can memorize (type 0 priest, 1 wizard).
class AssertSpellSlotsCommand : public ShellCommand {
public:
	AssertSpellSlotsCommand()
		: ShellCommand(
			"Assert-SpellSlots",
			{
				{ PARAMETER_STRING, },	// actor
				{ PARAMETER_INT, },	// type
				{ PARAMETER_INT, },	// spell level
				{ PARAMETER_INT, }	// expected count
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const int32 type = params.at(1).value.integer;
		const int32 level = params.at(2).value.integer;
		int32 found = -1;
		for (const cre_spell_memorization_info& info : actor->CRE()->SpellMemorizationInfo()) {
			if (info.type == type && info.level == level)
				found = info.numMemorizable;
		}
		if (found == params.at(3).value.integer) {
			std::cout << "ASSERT OK: " << actorName << " has " << found << " slot(s) of type "
				<< type << " level " << level << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << actorName << " has " << found << " slot(s) of type "
				<< type << " level " << level << ", expected " << params.at(3).value.integer
				<< std::endl;
		}
	}
};


// Assert-ClassLevel <actor>,<slot>,<level> - the level in class slot 0-2 (the
// "_"-separated parts of the class, in order: FIGHTER_THIEF is fighter, thief).
class AssertClassLevelCommand : public ShellCommand {
public:
	AssertClassLevelCommand()
		: ShellCommand(
			"Assert-ClassLevel",
			{
				{ PARAMETER_STRING, },	// actor
				{ PARAMETER_INT, },	// slot
				{ PARAMETER_INT, }	// level
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const int32 slot = params.at(1).value.integer;
		const int32 level = actor->CRE()->ClassLevel((uint8)slot);
		if (level == params.at(2).value.integer) {
			std::cout << "ASSERT OK: " << actorName << " class level " << slot << " == "
				<< level << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << actorName << " class level " << slot << " is "
				<< level << ", expected " << params.at(2).value.integer << std::endl;
		}
	}
};


// Assert-Sound <resource>,<channels>,<sampleRate> - the sound (a WAV/WAVC
// resource) decodes to 16-bit-or-8-bit PCM with these parameters and is not
// silent. Dump-Sound <resource>,<path> writes what it decodes to a RIFF WAV
// file, to listen to it.
static bool
_DecodeSound(const std::string& name, std::vector<uint8>& pcm, uint16& channels,
	uint16& bitsPerSample, uint32& sampleRate)
{
	WAVResource* wav = gResManager->GetWAV(name.c_str());
	if (wav == NULL)
		return false;
	const bool ok = wav->DecodePCM(pcm, channels, bitsPerSample, sampleRate);
	gResManager->ReleaseResource(wav);
	return ok;
}


class AssertSoundCommand : public ShellCommand {
public:
	AssertSoundCommand()
		: ShellCommand(
			"Assert-Sound",
			{
				{ PARAMETER_STRING, },	// resource
				{ PARAMETER_INT, },	// channels
				{ PARAMETER_INT, }	// sample rate
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string name = params.at(0).value.string;
		std::vector<uint8> pcm;
		uint16 channels = 0, bits = 0;
		uint32 rate = 0;
		if (!_DecodeSound(name, pcm, channels, bits, rate)) {
			std::cout << "ASSERT FAIL: sound " << name << " doesn't decode" << std::endl;
			return;
		}
		int32 peak = 0;
		if (bits == 16) {
			for (size_t i = 0; i + 1 < pcm.size(); i += 2) {
				int32 sample = (int16)(pcm[i] | (pcm[i + 1] << 8));
				peak = std::max<int32>(peak, sample < 0 ? -sample : sample);
			}
		} else {
			for (uint8 byte : pcm)
				peak = std::max<int32>(peak, byte > 128 ? byte - 128 : 128 - byte);
		}
		if (channels != (uint16)params.at(1).value.integer
				|| rate != (uint32)params.at(2).value.integer) {
			std::cout << std::dec << "ASSERT FAIL: sound " << name << " is " << channels
				<< " channel(s) at " << rate << " Hz" << std::endl;
		} else if (pcm.empty() || peak == 0) {
			std::cout << "ASSERT FAIL: sound " << name << " is silent" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT OK: sound " << name << " (" << pcm.size()
				<< " bytes, peak " << peak << ")" << std::endl;
		}
	}
};


// Assert-LastSound <resource> - the last sound the game asked to play (a voiced
// dialog line, a container's opening, ...), see Core::LastSoundPlayed().
class AssertLastSoundCommand : public ShellCommand {
public:
	AssertLastSoundCommand()
		: ShellCommand("Assert-LastSound")
	{
	}
	virtual void operator()(const char* argv) {
		const std::string found = Core::Get()->LastSoundPlayed();
		if (strcasecmp(found.c_str(), argv) == 0)
			std::cout << "ASSERT OK: last sound " << found << std::endl;
		else
			std::cout << "ASSERT FAIL: last sound \"" << found << "\", expected \""
				<< argv << "\"" << std::endl;
	}
};


class DumpSoundCommand : public ShellCommand {
public:
	DumpSoundCommand()
		: ShellCommand(
			"Dump-Sound",
			{
				{ PARAMETER_STRING, },	// resource
				{ PARAMETER_STRING, }	// output path
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		std::vector<uint8> pcm;
		uint16 channels = 0, bits = 0;
		uint32 rate = 0;
		if (!_DecodeSound(params.at(0).value.string, pcm, channels, bits, rate)) {
			std::cout << "Dump-Sound: FAILED" << std::endl;
			return;
		}
		std::ofstream file(params.at(1).value.string, std::ios::binary);
		auto put32 = [&] (uint32 v) { file.write(reinterpret_cast<const char*>(&v), 4); };
		auto put16 = [&] (uint16 v) { file.write(reinterpret_cast<const char*>(&v), 2); };
		file.write("RIFF", 4);
		put32(36 + (uint32)pcm.size());
		file.write("WAVEfmt ", 8);
		put32(16);
		put16(1);
		put16(channels);
		put32(rate);
		put32(rate * channels * bits / 8);
		put16((uint16)(channels * bits / 8));
		put16(bits);
		file.write("data", 4);
		put32((uint32)pcm.size());
		file.write(reinterpret_cast<const char*>(pcm.data()), (std::streamsize)pcm.size());
		std::cout << std::dec << "Dump-Sound: " << pcm.size() << " bytes, " << channels
			<< " channel(s), " << bits << " bit, " << rate << " Hz" << std::endl;
	}
};


// Assert-PaperdollSize <actor>,<letter> - the body-size letter the actor's
// paperdoll overlays (weapon, shield, helmet) are named with; "-" for a doll
// that takes none.
class AssertPaperdollSizeCommand : public ShellCommand {
public:
	AssertPaperdollSizeCommand()
		: ShellCommand(
			"Assert-PaperdollSize",
			{
				{ PARAMETER_STRING, }, // actor
				{ PARAMETER_STRING, }  // expected letter
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		const std::string actorName = params.at(0).value.string;
		std::string expected = params.at(1).value.string;
		if (expected == "-")
			expected.clear();
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const std::string size = actor->PaperdollSizeCode();
		if (size != expected) {
			std::cout << "ASSERT FAIL: " << actorName << " paperdoll size - expected '"
				<< expected << "', got '" << size << "'" << std::endl;
		} else {
			std::cout << "ASSERT OK: " << actorName << " paperdoll size '" << size
				<< "'" << std::endl;
		}
	}
};


static const char*
_TargetModeName(ActionBar::TargetMode mode)
{
	switch (mode) {
		case ActionBar::TARGET_TALK:
			return "talk";
		case ActionBar::TARGET_ATTACK:
			return "attack";
		case ActionBar::TARGET_CAST:
			return "cast";
		case ActionBar::TARGET_USE_ITEM:
			return "use";
		case ActionBar::TARGET_DEFEND:
			return "defend";
		case ActionBar::TARGET_NONE:
			break;
	}
	return "none";
}


// Assert-TargetMode <none|talk|attack|cast|use|defend> - the action bar's pending click mode.
class AssertTargetModeCommand : public ShellCommand {
public:
	AssertTargetModeCommand()
		: ShellCommand("Assert-TargetMode")
	{
	}
	virtual void operator()(const char* argv) {
		const std::string expected = argv;
		const ActionBar::TargetMode mode = Game::Get()->Bar().CurrentTargetMode();
		const char* actual = _TargetModeName(mode);
		if (strcasecmp(expected.c_str(), actual) == 0)
			std::cout << "ASSERT OK: target mode == " << actual << std::endl;
		else
			std::cout << "ASSERT FAIL: target mode - expected " << expected
				<< ", got " << actual << std::endl;
	}
};


// Assert-ActiveWeaponSlot <actor>,<CRE slot|-1> - which quickslot is in hand.
class AssertActiveWeaponSlotCommand : public ShellCommand {
public:
	AssertActiveWeaponSlotCommand()
		: ShellCommand("Assert-ActiveWeaponSlot")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, slotText;
		if (!_SplitOnFirstComma(argv, actorName, slotText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<slot>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const int32 expected = atoi(slotText.c_str());
		const int32 slot = actor->ActiveWeaponSlot();
		if (slot == expected)
			std::cout << "ASSERT OK: active weapon slot == " << expected << std::endl;
		else
			std::cout << "ASSERT FAIL: active weapon slot - expected " << expected
				<< ", got " << slot << std::endl;
	}
};


// Assert-ItemCount <actor>,<item resref>,<n> - total quantity of an item
// across all of an actor's slots (0 = not carried). "<n" asserts a count
// strictly below n (for quantities that depend on random rolls).
class AssertItemCountCommand : public ShellCommand {
public:
	AssertItemCountCommand()
		: ShellCommand("Assert-ItemCount")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, rest, itemName, countText;
		if (!_SplitOnFirstComma(argv, actorName, rest)
				|| !_SplitOnFirstComma(rest.c_str(), itemName, countText)) {
			std::cout << "ASSERT FAIL: expected <actor>,<item>,<n>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		int32 total = 0;
		for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
			IE::item item;
			if (actor->CRE()->GetItemAtSlot(slot, item)
					&& strcasecmp(item.name.CString(), itemName.c_str()) == 0)
				total += item.quantity1 > 0 ? item.quantity1 : 1;
		}
		const bool below = !countText.empty() && countText[0] == '<';
		const int32 expected = atoi(countText.c_str() + (below ? 1 : 0));
		if (below ? total < expected : total == expected)
			std::cout << std::dec << "ASSERT OK: " << itemName << " count " << (below ? "< " : "== ")
				<< expected << std::endl;
		else
			std::cout << std::dec << "ASSERT FAIL: " << itemName << " count - expected "
				<< (below ? "< " : "") << expected << ", got " << total << std::endl;
	}
};


// Assert-ItemAtSlot <actor>,<slot>,<item> - the CRE item slot holds this item
// (an empty <item> asserts that the slot is empty).
class AssertItemAtSlotCommand : public ShellCommand {
public:
	AssertItemAtSlotCommand()
		: ShellCommand("Assert-ItemAtSlot")
	{
	}
	virtual void operator()(const char* argv) {
		std::string actorName, rest, slotText, itemName;
		if (!_SplitOnFirstComma(argv, actorName, rest)
				|| !_SplitOnFirstComma(rest.c_str(), slotText, itemName)) {
			std::cout << "ASSERT FAIL: expected <actor>,<slot>,<item>" << std::endl;
			return;
		}
		Actor* actor = FindActor(actorName.c_str());
		if (actor == NULL || actor->CRE() == NULL) {
			std::cout << "ASSERT FAIL: no actor " << actorName << std::endl;
			return;
		}
		const uint32 slot = static_cast<uint32>(atoi(slotText.c_str()));
		IE::item item;
		const bool filled = actor->CRE()->GetItemAtSlot(slot, item);
		const std::string found = filled ? item.name.CString() : "";
		if (strcasecmp(found.c_str(), itemName.c_str()) == 0) {
			std::cout << std::dec << "ASSERT OK: slot " << slot << " holds \""
				<< itemName << "\"" << std::endl;
		} else {
			std::cout << std::dec << "ASSERT FAIL: slot " << slot << " - expected \""
				<< itemName << "\", got \"" << found << "\"" << std::endl;
		}
	}
};


// Assert-StoreWindow <true|false> - whether the store window is open.
class AssertStoreWindowCommand : public ShellCommand {
public:
	AssertStoreWindowCommand()
		: ShellCommand("Assert-StoreWindow")
	{
	}
	virtual void operator()(const char* argv) {
		bool expected = strcasecmp(argv, "true") == 0;
		bool open = Game::Get()->Screens().Find<StoreScreen>()->IsOpen();
		if (open == expected) {
			std::cout << "ASSERT OK: StoreWindow open == " << (expected ? "true" : "false")
				<< std::endl;
		} else {
			std::cout << "ASSERT FAIL: StoreWindow open - expected "
				<< (expected ? "true" : "false") << ", got "
				<< (open ? "true" : "false") << std::endl;
		}
	}
};


// Assert-StoreStock <store>,<item resref>,<amount> - how many packs of an
// item a store (opened earlier this session) currently holds; -1 for an
// infinite supply, 0 for none.
class AssertStoreStockCommand : public ShellCommand {
public:
	AssertStoreStockCommand()
		: ShellCommand("Assert-StoreStock")
	{
	}
	virtual void operator()(const char* argv) {
		std::string storeName, rest, itemName, expectedText;
		if (!_SplitOnFirstComma(argv, storeName, rest)
				|| !_SplitOnFirstComma(rest.c_str(), itemName, expectedText)) {
			std::cout << "ASSERT FAIL: expected <store>,<item>,<amount>" << std::endl;
			return;
		}
		int32 expected = (int32)::strtol(expectedText.c_str(), NULL, 0);

		Store* store = Game::Get()->Screens().Find<StoreScreen>()->LoadedStore(storeName.c_str());
		if (store == NULL) {
			std::cout << "ASSERT FAIL: store " << storeName << " isn't loaded" << std::endl;
			return;
		}
		int32 amount = 0;
		for (const store_entry& entry : store->Items()) {
			if (strcasecmp(entry.item.name.CString(), itemName.c_str()) == 0)
				amount = entry.amount;
		}
		if (amount == expected) {
			std::cout << "ASSERT OK: " << storeName << " stocks " << itemName << " x"
				<< amount << std::endl;
		} else {
			std::cout << "ASSERT FAIL: " << storeName << " stocks " << itemName
				<< " - expected " << expected << ", got " << amount << std::endl;
		}
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
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		Object* target = room->GetObject(targetName.c_str());
		if (target == NULL) {
			std::cout << "Click-Object: target \"" << targetName << "\" not found." << std::endl;
			return;
		}
		actor->ClickedOn(target);
	}
};


// Headless click at an area coordinate (unlike Mouse-Click, no camera/
// screen conversion needed) - exercises AreaRoom's own click dispatch
// (travel/info regions, doors, actors, ground piles) directly.
class ClickAreaCommand : public ShellCommand {
public:
	ClickAreaCommand()
		: ShellCommand("Click-Area", { { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point point = params.at(0).value.point;
		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		room->ClickAt(point);
	}
};


// Sets the current room's camera offset directly (area coordinates of
// the viewport's top-left corner) - headless testing has no other way
// to position the camera precisely (MOVETOCENTEROFSCREEN moves an
// actor *to* the existing camera center, it doesn't move the camera).
class SetCameraCommand : public ShellCommand {
public:
	SetCameraCommand()
		: ShellCommand("Set-Camera", { { PARAMETER_POINT, } })
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		IE::point point = params.at(0).value.point;
		Core::Get()->CurrentRoom()->SetAreaOffset(point);
	}
};


// PrintCameraCommand - reads back the current room's own area offset
// (Set-Camera's counterpart), for tests that need to confirm a
// scripted camera move (e.g. MoveViewObject) actually happened.
class PrintCameraCommand : public ShellCommand {
public:
	PrintCameraCommand()
		: ShellCommand("Print-Camera")
	{
	}
	virtual void operator()(const char* argv) {
		IE::point point = Core::Get()->CurrentRoom()->AreaOffset();
		std::cout << "Camera: (" << std::dec << point.x << "," << point.y << ")" << std::endl;
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
						<< " x" << item.quantity1 << " flags 0x" << std::hex << item.flags
						<< std::dec;
				ITMResource* itm = gResManager->GetITM(item.name);
				if (itm != NULL) {
					std::cout << " type " << itm->ItemType();
					gResManager->ReleaseResource(itm);
				}
				std::cout << std::endl;
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
		bool ok = Game::Get()->Saves().Save(params.at(0).value.string);
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
		bool ok = Game::Get()->Saves().Load(params.at(0).value.string);
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
			// SelectOption() can synchronously terminate the dialog (e.g.
			// a transition whose actions include STARTCUTSCENE) - `dialog`
			// may already be a dangling pointer here. Same hazard (and fix)
			// as TextArea::MouseDown()'s own dialog-option handling.
			if (!Game::Get()->InDialogMode())
				return;
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

		AreaRoom* room = CurrentAreaRoom();
		if (room == NULL)
			return;
		Object* object = room->GetObject(creatureName.c_str());
		if (object == NULL)
			return;

		action_params* actionParams = new action_params;
		actionParams->id = 153;
		actionParams->integer1 = enemyAlly;
		object->AddAction(actionParams);
		actionParams->Release();
	};
};


class SetReputationCommand : public ShellCommand {
public:
	SetReputationCommand()
		: ShellCommand(
			"Set-Reputation",
			{
			   { PARAMETER_INT, }
			}
		)
	{
	}
	virtual void operator()(const char* argv) {
		const ShellCommandParameters params = ParseParameters(argv);
		uint16 reputation = params.at(0).value.integer;
		action_params* actionParams = new action_params;
		actionParams->id = 153;
		actionParams->integer1 = reputation;
		AreaRoom* room = CurrentAreaRoom();
		if (room != NULL)
			room->AddAction(actionParams);
		actionParams->Release();
	}
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
	console->AddCommand(new ListDoorsCommand());
	console->AddCommand(new ListRegionsCommand());
	console->AddCommand(new ListGroundCommand());
	console->AddCommand(new PickUpGroundCommand());
	console->AddCommand(new CharNewCommand());
	console->AddCommand(new CharSetCommand());
	console->AddCommand(new CharRollCommand());
	console->AddCommand(new CharAbilityCommand());
	console->AddCommand(new CharPrintCommand());
	console->AddCommand(new CharBuildCommand());
	console->AddCommand(new ListResourcesCommand());
	console->AddCommand(new MoveViewPointCommand());
	console->AddCommand(new PrintObjectCommand());
	console->AddCommand(new PrintVariablesCommand());
	console->AddCommand(new PrintJournalCommand());
	console->AddCommand(new SetEnemyAllyCommand());
	console->AddCommand(new SetReputationCommand());
	console->AddCommand(new ShowWindowCommand());
	console->AddCommand(new ToggleAuxWindowCommand());
	console->AddCommand(new ShakeScreenCommand());
	console->AddCommand(new ScreenshotCommand());
	console->AddCommand(new ToggleInventoryCommand());
	console->AddCommand(new ToggleRecordCommand());
	console->AddCommand(new ToggleHUDCommand());
	console->AddCommand(new SelectPartyCommand());
	console->AddCommand(new EvaluateTriggerCommand());
	console->AddCommand(new EvaluateTriggersCommand());
	console->AddCommand(new AssertTriggerCommand());
	console->AddCommand(new AssertTriggersCommand());
	console->AddCommand(new QueueActionCommand());
	console->AddCommand(new CheckPassableCommand());
	console->AddCommand(new CheckWorldmapExitCommand());
	console->AddCommand(new AssertAreaMapVisibleCommand());
	console->AddCommand(new AssertDoorOpenedCommand());
	console->AddCommand(new AssertPositionCommand());
	console->AddCommand(new AssertJournalHasEntryCommand());
	console->AddCommand(new AssertJournalSectionCommand());
	console->AddCommand(new AssertContainerHasItemCommand());
	console->AddCommand(new AssertLootWindowCommand());
	console->AddCommand(new PrintStoreCommand());
	console->AddCommand(new PrintGamCommand());
	console->AddCommand(new CheckDialogsCommand());
	console->AddCommand(new AssertGamNPCCommand());
	console->AddCommand(new AssertPortraitCountCommand());
	console->AddCommand(new AssertActorCountCommand());
	console->AddCommand(new AssertItemIdentifiedCommand());
	console->AddCommand(new SelectWeaponCommand());
	console->AddCommand(new SelectActorCommand());
	console->AddCommand(new ToggleSelectedCommand());
	console->AddCommand(new DragSelectCommand());
	console->AddCommand(new AssertSelectedCommand());
	console->AddCommand(new AssertSelectedCountCommand());
	console->AddCommand(new AssertTargetModeCommand());
	console->AddCommand(new AssertActorHasColorsCommand());
	console->AddCommand(new AssertScreenOpenCommand());
	console->AddCommand(new PrintDialogCommand());
	console->AddCommand(new AssertPausedCommand());
	console->AddCommand(new AssertDialogActiveCommand());
	console->AddCommand(new AssertDialogFileCommand());
	console->AddCommand(new AssertItemAtSlotCommand());
	console->AddCommand(new AssertPaperdollCommand());
	console->AddCommand(new AssertPaperdollSizeCommand());
	console->AddCommand(new AssertSoundCommand());
	console->AddCommand(new PlayMovieCommand());
	console->AddCommand(new AssertMovieSkipCommand());
	console->AddCommand(new PlayMusicFileCommand());
	console->AddCommand(new PlayPlaylistCommand());
	console->AddCommand(new EndPlaylistCommand());
	console->AddCommand(new PrintPlaylistCommand());
	console->AddCommand(new AssertMusicTrackCommand());
	console->AddCommand(new AssertPlaylistNextCommand());
	console->AddCommand(new CheckMusicCommand());
	console->AddCommand(new StopMusicCommand());
	console->AddCommand(new PrintMusicCommand());
	console->AddCommand(new AssertMusicCommand());
	console->AddCommand(new AssertMusicPositionCommand());
	console->AddCommand(new WaitAudioCommand());
	console->AddCommand(new DumpMusicFileCommand());
	console->AddCommand(new AssertCanLevelUpCommand());
	console->AddCommand(new AssertClassLevelCommand());
	console->AddCommand(new LevelUpCommand());
	console->AddCommand(new AssertSpellSlotsCommand());
	console->AddCommand(new AssertLastSoundCommand());
	console->AddCommand(new DumpSoundCommand());
	console->AddCommand(new AssertCustomColorsCommand());
	console->AddCommand(new AssertActiveWeaponSlotCommand());
	console->AddCommand(new AssertItemCountCommand());
	console->AddCommand(new AssertStoreWindowCommand());
	console->AddCommand(new AssertStoreStockCommand());
	console->AddCommand(new CheckLineOfSightCommand());
	console->AddCommand(new ToggleSearchMapCommand());
	console->AddCommand(new ToggleSaveCommand());
	console->AddCommand(new ToggleLoadCommand());
	console->AddCommand(new ToggleJournalCommand());
	console->AddCommand(new ToggleArcaneSpellbookCommand());
	console->AddCommand(new ToggleDivineSpellbookCommand());
	console->AddCommand(new InvokeControlCommand());
	console->AddCommand(new RightClickControlCommand());
	console->AddCommand(new MouseDragCommand());
	console->AddCommand(new MouseClickCommand());
	console->AddCommand(new MouseMoveCommand());
	console->AddCommand(new WaitTimeCommand());

	console->AddCommand(new WalkToObjectCommand());
	console->AddCommand(new ClickObjectCommand());
	console->AddCommand(new ClickAreaCommand());
	console->AddCommand(new SetCameraCommand());
	console->AddCommand(new PrintCameraCommand());
	console->AddCommand(new DisplayStringCommand());
	console->AddCommand(new ShowCharacterCommand());

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
