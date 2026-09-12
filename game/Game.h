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
class ARAResource;
class CharacterBuilder;
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
	// Original game's TAB-key HUD toggle (portraits, action menu, message
	// area) - unlike GUI::Hide()/Show()/Toggle(), the mouse cursor stays
	// visible.
	void ToggleHUD();

	// Switches which party member mouse clicks/queued actions control -
	// real BG2's number-key (1-6)/portrait-click party selection; this
	// engine only wires the number-key side so far (no portrait bar to
	// click yet). No-op if index is out of range or the current room
	// isn't an AreaRoom (e.g. the worldmap screen).
	void SelectPartyMember(uint16 index);

	void ToggleInventoryWindow();
	void ToggleRecordWindow();
	// Minimal single-slot Save/Load screens (GUISAVE/GUILOAD) - real BG2
	// has a full multi-slot browser (portraits, names, dates, delete) in
	// those same CHU files, not modeled here; only the one slot's name/
	// status labels and the main confirm button are wired, same spirit
	// as other "real data, reduced scope" simplifications already on the
	// roadmap.
	void ToggleSaveWindow();
	void ToggleLoadWindow();
	// GUI::ControlInvoked() routes clicks on GUISAVE/GUILOAD controls
	// here (Game owns Save()/Load(), GUI doesn't reach into game state).
	void SaveOrLoadControlInvoked(const res_ref& chuName, uint32 controlID,
		uint16 windowID);
	// GUI::ControlInvoked() routes clicks on GUIINV (Inventory) slot
	// buttons here - click-to-pick-up, click-to-drop-and-swap between
	// item slots. See the .cpp.
	void InventoryControlInvoked(uint32 controlID, uint16 windowID);
	// Right-click on an inventory slot: examine the item (opens the
	// GUIINVHI info window), or cancel an in-progress drag.
	void InventoryControlRightClicked(uint32 controlID, uint16 windowID);
	// Hover enter/leave on an inventory slot: show/hide the item-name
	// tooltip next to the cursor.
	void InventoryControlHovered(uint32 controlID, uint16 windowID, bool inside);
	// A click on the inventory window background (not a slot) while
	// dragging an item: drops the held item onto the area floor at the
	// shown character's feet.
	void DropHeldItemOnGround();
	// GUI::ControlInvoked() routes clicks on GUIREC (Record screen)
	// controls here - currently just the portrait column.
	void RecordControlInvoked(uint32 controlID, uint16 windowID);
	// Switches which party member the Inventory / Record screens show
	// (portrait-column click). Also selects them in the world so the two
	// stay in sync. No-op for an out-of-range index.
	void ShowCharacter(uint16 partyIndex);

	// (Re)draws the HUD portrait bar (GUIW's WINDOW_PLAYER_SLOTS) from the
	// current party. Call after an area load rebuilds the HUD.
	void RefreshHUDPortraits();
	void ToggleJournalWindow();
	// GUI::ControlInvoked() routes clicks on GUIJRNL controls here -
	// the command bar (window 0) and the portrait column (window 1),
	// same layout/handling as InventoryControlInvoked/RecordControlInvoked.
	void JournalControlInvoked(uint32 controlID, uint16 windowID);
	// Mage spellbook (GUIMG), read-only for now: shows the currently
	// displayed character's known + memorized arcane spells.
	void ToggleSpellbookWindow();
	void SpellbookControlInvoked(uint32 controlID, uint16 windowID);
	// Hover over a spellbook grid icon -> show the spell's name as a
	// cursor tooltip.
	void SpellbookControlHovered(uint32 controlID, bool inside);
	// Right-click a spellbook grid icon -> open the spell-info popup
	// (GUIMG/GUIPR window 3: name + description).
	void SpellbookControlRightClicked(uint32 controlID, uint16 windowID);

	// Queues RESTPARTY(230) on the first party member - same action
	// SETAREARESTFLAG/RunActionRestParty already implement (Fase 4/10),
	// just triggered from the HUD Rest button instead of a script.
	void TriggerRest();

	bool Load(const char* name);
	bool Save(const char* name);

	// Hands off a live Actor object between two AreaRoom loads within the
	// same running session (e.g. a scripted MOVEBETWEENAREASEFFECT) -
	// not disk persistence, that's GamResource/Save()'s job, and it only
	// covers party members serialized at the CREResource level (no live
	// runtime state like active effects or a pending action queue - see
	// GamResource.h's own header comment). Keyed by destination area
	// name (res_ref's operator< is case-insensitive, matching resref
	// conventions) so more than one pending handoff can be in flight at
	// once without actors from different destinations getting mixed up;
	// AreaRoom::_LoadActors() drains only its own area's entry.
	class TempState {
	public:
		struct PendingActor {
			Actor* actor;
			IE::point position;
			uint16 orientation;
		};
		std::map<res_ref, std::vector<PendingActor>> actors;
	};
	TempState* GetTempState();

	// A session-only cache of what an area's own non-party actors and
	// its ARAResource looked like the last time the party left it - real
	// BG2 checkpoints a modified copy of the area (in the save folder)
	// when you leave it, and restores from that on return; this engine
	// otherwise just discards the whole thing (see AreaRoom::_UnloadArea()),
	// losing anything that lives in the ARE's own resource data - a
	// placed actor's IE::actor struct (position, NumTimesTalkedTo,
	// script names - Actor::fActor points directly into it) and a door's
	// IE::door struct (open/closed/locked - Door::fAreaDoor likewise) -
	// as well as anything only tracked on the C++ Actor/CREResource
	// objects themselves (HP, inventory, ...). Keeping the ARAResource
	// itself alive (not released) covers the first kind for free (doors
	// included, even though nothing here is door-specific); keeping the
	// actors themselves alive (not released) covers the second.
	// AreaRoom::_UnloadArea() populates one entry per area name (even if
	// empty - an empty `actors` on a *revisit* correctly means "everyone
	// here died/left last time", as opposed to a missing entry meaning
	// "never visited, parse fresh from the resource"); AreaRoom::
	// _LoadActors()/its constructor consume it. Not persisted anywhere -
	// gone once the process exits (cleaned up in ~Game(), same as
	// everything else still live at that point).
	class AreaCache {
	public:
		struct CachedArea {
			ARAResource* area = nullptr;
			std::vector<Actor*> actors;
			// Items dropped on the floor while the party was here.
			std::vector<IE::ground_pile> groundPiles;
			// Each container's remaining contents (keyed by its index in
			// the area's container list), so a looted chest stays looted
			// on re-entry.
			std::map<uint32, std::vector<IE::item>> containerContents;
		};
		std::map<res_ref, CachedArea> areas;
	};
	AreaCache* GetAreaCache();

	// Headless character-creation state (roadmap Fase 47). Driven by the
	// Char-* console commands; A2 will turn a completed one into the
	// starting-party protagonist.
	CharacterBuilder& GetCharacterBuilder();

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

	// Path to a character-creation spec file. When set, CreateParty()
	// builds the party leader from it (via CharacterBuilder), injects
	// the resulting CRE as "PLAYER1", and starts with that + the usual
	// companion instead of the hardcoded default party.
	void SetCharacterSpec(const char* path);

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
	AreaCache* fAreaCache;
	CharacterBuilder* fCharBuilder;

	uint32 fDelay;
	bool fTestMode;

	std::vector<std::string> fStartingPartyMembers;
	std::string fExecFile;
	std::string fStartingArea;
	std::string fCharacterSpec;

	// Parses fCharacterSpec into fCharBuilder, builds the CRE, injects it
	// as "PLAYER1", and adds it (plus the default companion) to fParty.
	// Returns false (and adds nothing) on any parse/build failure.
	bool _CreateCharacterFromSpec(const IE::point& position);

	std::map<std::string, std::string> fTokens;
	std::vector<uint32> fJournalEntries;
	std::map<std::string, bool> fAreaMapVisibility;

	// CRE item-slot the player is currently dragging an inventory item
	// out of (-1 = not dragging). The dragged icon itself lives on GUI
	// (SetDragBitmap); this is the model side.
	int32 fInvDragSlot;

	// Party index whose sheet the Inventory / Record screens show.
	uint16 fShownCharacter;

	void _RunExecFile(GameConsole* console);
	// The party member the Inventory/Record screens currently show (see
	// fShownCharacter), or NULL if the party is empty / the index is
	// stale.
	Actor* _ShownActor() const;
	// Fills a portrait column (buttons id 0..count-1) with the party's
	// small portraits - the HUD bar and the Inventory/Record side panel
	// share this.
	void _UpdatePortraitColumn(class Window* window, uint32 count);
	void _UpdateSpellbookScreen();
	void _ShowSpellInfo(const res_ref& spellName);
	// GUIMG/GUIPR grid control id -> the spell resref it currently shows,
	// plus which of the two CHUs the open spellbook is.
	std::string fSpellbookCHU;
	uint16 fSpellbookLevel = 1;
	std::map<uint32, res_ref> fSpellbookKnown;
	std::map<uint32, res_ref> fSpellbookMemo;
	// Re-populates the Inventory / Record screens (whichever are open)
	// after fShownCharacter changes.
	void _RefreshCharacterScreens();
	void _UpdateInventoryIcons();
	void _SetSlotIcon(class Window* window, class CREResource* cre,
		uint32 controlID, uint32 creSlot);
	void _UpdateGroundItemSlots(class Window* window, Actor* actor);
	void _ShowItemInfo(const res_ref& itemName);
	void _UpdatePaperdoll(class Window* window, Actor* actor);
	void _UpdateInventoryLabels(class Window* window, Actor* actor);
	void _UpdateRecordLabels();
	void _UpdateAbilityScoreLabels(class Window* window, class CREResource* cre);
	void _UpdateClassRaceLevelLabels(class Window* window, Actor* actor);
	void _UpdateSavesAndResistances(class Window* window, class CREResource* cre);
	std::string _TitleCaseIDSName(const std::string& idsName);
	void _UpdateSaveLoadLabels(const res_ref& chuName);
	void _UpdateJournalLabels();
};

#endif /* GAME_H_ */
