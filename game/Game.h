/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: Stefano Ceccherini
 */

#ifndef GAME_H_
#define GAME_H_

#include "IETypes.h"

#include <map>
#include <string>
#include <vector>

#define AI_UPDATE_FREQ 15
#define ROUND_DURATION_SEC 6


class Actor;
class DialogHandler;
class GameConsole;
class Party;
class Game {
public:
	static Game* Get();
	void Loop(bool noNewGame = false, bool executeScripts = true);
	void CreateParty();
	void InitiateDialog(Actor* actor, Actor* target);
	bool InDialogMode() const;
	void TerminateDialog();
	DialogHandler* Dialog();

	::Party* Party();

	void LoadStartingArea();
	void ToggleDayNight();
	void ToggleInventoryWindow();
	void ToggleRecordWindow();

	bool Load(const char* name);
	bool Save(const char* name);

	// TODO: we need this to keep track of actor moving between areas
	// until we have proper support for saving areas data
	class TempState {
	public:
		std::vector<Actor*> actors;
	};
	TempState* GetTempState();

	void SetTestMode(bool value);
	bool TestMode() const;

	// Overrides CreateParty()'s hardcoded starting party with this list of
	// CRE resrefs (loaded in order, all at the same default spawn point) -
	// set from the command line (see bgemu.cpp's --party option). Leaving
	// this empty keeps CreateParty()'s original hardcoded default.
	void SetStartingPartyMembers(const std::vector<std::string>& names);

	// Path to a shell-command test script (see bgemu.cpp's --exec-file
	// option): one GameConsole command per line, blank lines and lines
	// starting with '#' ignored, run automatically right after the
	// starting area/worldmap loads. Meant for unattended/headless test
	// runs - once the script finishes, the game quits (as if 'q' had
	// been pressed) rather than entering the normal interactive loop.
	// Leaving this unset (NULL/empty) runs the game normally.
	void SetExecFile(const char* path);

	// Overrides the normal startup flow (opening cutscene via
	// LoadStartingArea(), or the worldmap with --no-newgame) to load this
	// area resref directly instead - set from the command line (see
	// bgemu.cpp's --area option). Takes priority over --no-newgame; both
	// skip the party-placement/STARTPOS logic LoadStartingArea() does, so
	// the party spawns wherever the area's own (entranceless) default
	// position is. Leaving this empty keeps the normal startup flow.
	void SetStartingArea(const char* areaName);

	// Dialog placeholder tokens (SETTOKEN/SETTOKENOBJECT/SETGABBER) -
	// resolved by DialogHandler::_FillPlaceHolders() alongside the
	// existing hardcoded <CHARNAME>. Per IESDP, values set this way
	// aren't persisted to the savegame - this engine doesn't persist
	// them either, they just live for the current session.
	void SetToken(const std::string& name, const std::string& value);
	const std::map<std::string, std::string>& Tokens() const;

	// Minimal in-memory journal (ADDJOURNALENTRY/ERASEJOURNALENTRY/
	// SETQUESTDONE) - no GUI screen consumes this yet (see the Fase 6
	// plan notes on Journal never being built), and no section
	// distinction (Quest/Story/User) is modeled - just an ordered list
	// of strrefs, closest to the "User" section in spirit.
	void AddJournalEntry(uint32 strref);
	void RemoveJournalEntry(uint32 strref);
	const std::vector<uint32>& JournalEntries() const;

	// REVEALAREAONMAP/HIDEAREAONMAP - kept here rather than on the
	// AreaEntry/WorldMap objects directly, since WorldMap is recreated
	// fresh (re-reading the WMAP resource) every time the player opens
	// the worldmap screen - see Core::LoadWorldMap(). WorldMap::
	// _LoadAreaEntries() applies this on top of each AreaEntry's own
	// file-driven visibility bit right after loading it.
	void SetAreaMapVisible(const std::string& areaName, bool visible);
	// Returns true and sets *visible if a script overrode this area's
	// worldmap visibility; false (leaving *visible untouched) if it
	// should keep the file's own flag.
	bool AreaMapVisibleOverride(const std::string& areaName, bool* visible) const;


private:
	Game();
	~Game();

	DialogHandler* fDialog;

	::Party* fParty;
	TempState* fTempState;

	uint32 fDelay;
	bool fTestMode;

	std::vector<std::string> fStartingPartyMembers;
	std::string fExecFile;
	std::string fStartingArea;

	std::map<std::string, std::string> fTokens;
	std::vector<uint32> fJournalEntries;
	std::map<std::string, bool> fAreaMapVisibility;

	void _RunExecFile(GameConsole* console);
	void _UpdateInventoryIcons();
	void _SetSlotIcon(class Window* window, class CREResource* cre,
		uint32 controlID, uint32 creSlot);
	void _UpdateInventoryLabels(class Window* window, Actor* actor);
	void _UpdateRecordLabels();
	std::string _TitleCaseIDSName(const std::string& idsName);
};

#endif /* GAME_H_ */
