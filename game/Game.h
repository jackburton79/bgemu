/*
 * Game.h
 *
 *  Created on: 29/mar/2015
 *      Author: Stefano Ceccherini
 */

#pragma once

#include "IETypes.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#define AI_UPDATE_FREQ 15
#define ROUND_DURATION_SEC 6


class Actor;
class ScreenManager;
class ARAResource;
class CharacterBuilder;
class CREResource;
class DialogHandler;
class GameConsole;
class GamResource;
struct gam_party_member;
class Party;

class Game {
public:
	static Game* Get();
	void Loop(bool noNewGame = false, bool executeScripts = true);
	void InitiateDialog(Actor* actor, Actor* target);
	bool InDialogMode() const;
	void TerminateDialog();
	DialogHandler* Dialog();

	::Party* Party();

	// The global NPCs (see NPCRoster).
	class NPCRoster& NPCs();

	// An actor of the current area becomes a global NPC (MAKEGLOBAL): the
	// area stops placing it, the roster keeps it from now on (unless it is
	// in the party, or already one).
	void MakeNPC(Actor* actor);
	// Party membership: a global NPC that joins stops being one, a member
	// that leaves becomes one (staying where it is).
	void JoinParty(Actor* actor);
	void LeaveParty(Actor* actor);

	void LoadStartingArea();
	void ToggleDayNight();
	// Original game's TAB-key HUD toggle (portraits, action menu, message
	// area) - unlike GUI::Hide()/Show()/Toggle(), the mouse cursor stays
	// visible.
	void ToggleHUD();

	// Switches which party member mouse clicks/queued actions control -
	// both the number keys (1-6) and a left click on that member's HUD
	// portrait replace the whole map selection with just them, and become
	// the shown character (inventory/record/action bar). No-op if index
	// is out of range or the current room isn't an AreaRoom (e.g. the
	// worldmap screen).
	void SelectPartyMember(uint16 index);
	// Shift-click on a portrait: adds/removes just that member from the
	// map selection (AreaRoom::ToggleSelected()) without touching the
	// shown character - same convention as shift-clicking their avatar
	// on the map.
	void ToggleSelectedPartyMember(uint16 index);

	// Centers view on party member
	void CenterViewOnPartyMember(uint16 index);

	// The screens that are ported to GameScreen (see screens/); the rest
	// are still Game's own Toggle*Window() and friends.
	ScreenManager& Screens();
	// The party member the Inventory/Record screens currently show (see
	// fShownCharacter), or NULL if the party is empty / the index is
	// stale.
	Actor* ShownActor() const;
	// Fills a portrait column (buttons id 0..count-1) with the party's
	// small portraits - the HUD bar and the Inventory/Record side panel
	// share this.
	void UpdatePortraitColumn(class Window* window, uint32 count);

	// The loot window (see LootWindow).
	class LootWindow& Loot();
	// Called when an area is being unloaded (the source object dies with
	// it). Safe to call when no Game exists (shutdown).
	static void CloseContainerWindowIfAny();
	// Switches which party member the Inventory / Record screens show
	// (portrait-column click). Also selects them in the world so the two
	// stay in sync. No-op for an out-of-range index.
	void ShowCharacter(uint16 partyIndex);
	// Makes `actor` (if in the party) the character the screens show, without
	// touching the world selection.
	void SetShownActor(Actor* actor);

	// (Re)draws the HUD portrait bar (GUIW's WINDOW_PLAYER_SLOTS) from the
	// current party. Call after an area load rebuilds the HUD.
	void RefreshHUDPortraits();
	// The HUD action bar (see ActionBar).
	class ActionBar& Bar();



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

		// Where a party member asked (LEAVEAREALUA's point and face) to
		// stand in the area the party is about to enter, by the member's
		// global id. Consumed - and cleared - by that area's constructor.
		struct Placement {
			IE::point position;
			int orientation;
		};
		std::map<uint16, Placement> partyPlacements;
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
	// Releases every entry in the area cache (same balancing act as ~Game()
	// itself) and empties it - a load must drop the live session's own
	// cached area state before adopting a save's, or it would resurrect
	// whatever the *current*, unsaved session left behind for any area not
	// being loaded right now instead of that save's own history.
	void ClearAreaCache();
	// Replaces the party with a new, empty one (what loading a save does).
	::Party* ResetParty();

	// The party a new game starts with (see StartingParty).
	class StartingParty& Starting();

	void SetTestMode(bool value);
	bool TestMode() const;

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

	// Saving and loading (see SavedGame).
	class SavedGame& Saves();


	// Dialog placeholder tokens (SETTOKEN/SETTOKENOBJECT/SETGABBER) -
	// resolved by DialogHandler::_FillPlaceHolders() alongside the
	// existing hardcoded <CHARNAME>. Per IESDP, values set this way
	// aren't persisted to the savegame - this engine doesn't persist
	// them either, they just live for the current session.
	void SetToken(const std::string& name, const std::string& value);
	const std::map<std::string, std::string>& Tokens() const;

	// The journal's notes (see GameJournal).
	class GameJournal& Journal();

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
	class NPCRoster* fNPCs;
	TempState* fTempState;
	AreaCache* fAreaCache;
	class StartingParty* fStartingParty;

	uint32 fDelay;
	bool fTestMode;

	std::string fExecFile;
	std::string fStartingArea;

	// Releases every entry in fAreaCache (same balancing act as ~Game()
	// itself, which used to do this inline) and empties it - shared with
	// Load(), which must drop the live session's own cached area state
	// before adopting a save's, or a load would otherwise resurrect
	// whatever the *current*, unsaved session left behind for any area
	// not being loaded right now instead of that save's own history.


	std::map<std::string, std::string> fTokens;
	class GameJournal* fJournal;
	std::map<std::string, bool> fAreaMapVisibility;


	class LootWindow* fLoot;
	class ActionBar* fBar;


	// Party index whose sheet the Inventory / Record screens show.
	uint16 fShownCharacter;

	// Re-populates the Inventory / Record screens (whichever are open)
	// after fShownCharacter changes.
	void _RefreshCharacterScreens();
	ScreenManager* fScreens;
	class SavedGame* fSaves;
};
