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

// The Map/Journal/Inventory/Record/Spellbook/Save command-bar actions -
// identical (same control ids) on both the main HUD bar (gui/GUI.cpp's
// own extra entries) and the copy embedded in every full-screen panel's
// own window 0 (Game.cpp's own extra entries); each caller adds only
// its own extras (Pause on the HUD, Rest under a differing id) on top
// of this shared table.
struct CommandBarButton { uint32 controlID; void (*action)(); };

// One journal entry. `section` is one of Game::journal_section; `time` is in
// game seconds (GameTimer::GameTime()) at the moment the entry was added or
// last moved between sections, `chapter` the CHAPTER global then.
struct journal_entry {
	uint32 strref;
	uint8 section;
	uint8 group;
	uint8 chapter;
	uint32 time;
};
extern const CommandBarButton kCommandBarButtons[9];

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

	// Global NPCs: the creatures outside the party that the game itself
	// keeps track of (a new game's companions and story characters, or
	// whoever left the party) - unlike an area's other actors they aren't
	// part of any area's own state: each one just remembers the area and
	// point it is at (Actor::AreaName(), Position()) and shows up when
	// that area is loaded. AddNPC() takes over the caller's reference;
	// RemoveNPC() drops the list's.
	uint16 CountNPCs() const;
	Actor* NPCAt(uint16 index) const;
	bool IsNPC(const Actor* actor) const;
	// The NPC a script's object name refers to (its CRE name or death
	// variable), NULL if none.
	Actor* FindNPC(const char* name) const;
	void AddNPC(Actor* actor);
	void RemoveNPC(Actor* actor);

	// An actor of the current area becomes a global NPC (MAKEGLOBAL): the
	// area stops placing it, the Game keeps it from now on.
	void MakeNPC(Actor* actor);
	// Moves a global NPC to a point of an area, which needn't be loaded:
	// out of the room it is in if that isn't the destination, into the
	// current room if it is.
	void MoveNPC(Actor* npc, const res_ref& area, const IE::point& position,
		int orientation);
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
	// Hides every other screen, the old ones included - what opening one
	// does first.
	void CloseOtherScreens(const char* exceptCHU);
	// Highlights whichever command-bar icon (HUD bar and/or the copy
	// embedded in the open panel itself) corresponds to the currently
	// open full-screen panel.
	void UpdateCommandBarToggle();
	// A click on the command-bar copy embedded in a full-screen panel.
	static void AuxCommandBarInvoked(uint32 controlID);
	// The party member the Inventory/Record screens currently show (see
	// fShownCharacter), or NULL if the party is empty / the index is
	// stale.
	Actor* ShownActor() const;
	// Fills a portrait column (buttons id 0..count-1) with the party's
	// small portraits - the HUD bar and the Inventory/Record side panel
	// share this.
	void UpdatePortraitColumn(class Window* window, uint32 count);

	// Loot window (GUIW window 8, replacing the message area/command bar
	// at the bottom of the screen while open): `looter` is the party
	// member using `source`, either a Container or a dead Actor (corpse).
	// Shows the source's items on the left and the looter's own carried
	// items on the right; clicking one moves it across. See the .cpp.
	void OpenContainerWindow(Actor* looter, class Object* source);
	void CloseContainerWindow();
	bool IsContainerWindowOpen() const;
	// Called when an area is being unloaded (the source object dies with
	// it). Safe to call when no Game exists (shutdown).
	static void CloseContainerWindowIfAny();
	void ContainerControlInvoked(uint32 controlID);
	void ContainerControlHovered(uint32 controlID, bool inside);
	// Store window (GUISTORE) - the Buy/Sell page: opened by STARTSTORE for
	// a party member shopping (`customer`). Items on the shelf on the left,
	// the shown party member's carried items on the right; a click selects
	// or deselects an item, "Buy"/"Sell" then carry out the selection.
	// Returns false if `storeName` isn't a shop this window can show (no
	// such resource, or a tavern/inn/temple - their pages don't exist yet).
	bool OpenStoreWindow(Actor* customer, const res_ref& storeName);
	void CloseStoreWindow();
	bool IsStoreWindowOpen() const;
	// A store loaded this session (NULL if never opened) - for tests.
	class Store* LoadedStore(const char* name) const;
	void StoreControlInvoked(uint32 controlID, uint16 windowID);
	void StoreControlHovered(uint32 controlID, uint16 windowID, bool inside);
	// A double click on a store control (GUI detects it): on a shelf item,
	// opens the quantity picker (BG2 only - BG1's GUISTORE has none).
	// Returns whether it was handled.
	bool StoreControlDoubleClicked(uint32 controlID, uint16 windowID);
	// Switches which party member the Inventory / Record screens show
	// (portrait-column click). Also selects them in the world so the two
	// stay in sync. No-op for an out-of-range index.
	void ShowCharacter(uint16 partyIndex);

	// (Re)draws the HUD portrait bar (GUIW's WINDOW_PLAYER_SLOTS) from the
	// current party. Call after an area load rebuilds the HUD.
	void RefreshHUDPortraits();
	// Fills the HUD action bar (GUIW's WINDOW_CMDS, 12 buttons) for the
	// shown party member - the class's row of actions, with the four
	// weapon quickslots working. Call whenever the shown character, their
	// weapons or the HUD itself change.
	void RefreshActionBar();

	// What the next click in the area does, chosen from the action bar:
	// talk to / attack the creature clicked instead of the usual
	// friend-or-foe guess. One-shot: any click in the area ends it.
	enum TargetMode { TARGET_NONE, TARGET_TALK, TARGET_ATTACK, TARGET_CAST,
		TARGET_USE_ITEM, TARGET_DEFEND };
	TargetMode CurrentTargetMode() const { return fTargetMode; }
	void SetTargetMode(TargetMode mode);
	// Ends TARGET_CAST/TARGET_USE_ITEM: the shown character casts the spell
	// (or uses the item) picked from the action bar at `target`.
	void CastSpellAt(Actor* target);
	// GUI::ControlInvoked() routes clicks on the action bar here.
	void ActionBarControlInvoked(uint32 controlID);
	// A right click on a quick spell button offers the spell page to assign
	// one to it.
	void ActionBarControlRightClicked(uint32 controlID);

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

	// Directory holding everything this engine writes for saving: one
	// "savegame_slot<N>.gam" (+ ".arecache" directory) per slot and the
	// session's own area checkpoints ("current/arecache", see
	// AreaRoom::AreaCheckpointDir()). Set once at startup (bgemu.cpp),
	// per game installation.
	void SetSaveDirectory(const std::string& path);
	const std::string& SaveDirectory() const;
	std::string SaveSlotPath(uint32 index) const;

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

	// The journal: entries added by ADDJOURNALENTRY, moved/removed by
	// SETQUESTDONE/ERASEJOURNALENTRY, and by dialog transitions that carry
	// a journal note (DialogHandler). Semantics follow GemRB's Game::
	// AddJournalEntry(): an entry is unique per strref - adding one that
	// exists in the same section changes nothing (returns false), in
	// another section moves it there (or, finishing a quest that belongs
	// to a group, replaces the whole group with it).
	enum journal_section {
		JOURNAL_USER = 0,
		JOURNAL_QUEST = 1,
		JOURNAL_DONE = 2,
		JOURNAL_INFO = 4
	};
	bool AddJournalEntry(uint32 strref, uint8 section, uint8 group = 0);
	void RemoveJournalEntry(uint32 strref);
	void RemoveJournalGroup(uint8 group);
	const std::vector<journal_entry>& Journal() const;
	// Strrefs only, in order - for the console and tests.
	std::vector<uint32> JournalEntries() const;
	void SetJournal(const std::vector<journal_entry>& entries);

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
	std::vector<Actor*> fNPCs;
	TempState* fTempState;
	AreaCache* fAreaCache;
	CharacterBuilder* fCharBuilder;

	uint32 fDelay;
	bool fTestMode;

	std::vector<std::string> fStartingPartyMembers;
	std::string fExecFile;
	std::string fStartingArea;
	std::string fSaveDirectory;
	std::string fCharacterSpec;

	// Parses fCharacterSpec into fCharBuilder, builds the CRE, injects it
	// as "PLAYER1", and adds it (plus the default companion) to fParty.
	// Returns false (and adds nothing) on any parse/build failure.
	bool _CreateCharacterFromSpec(const IE::point& position);

	// Releases every entry in fAreaCache (same balancing act as ~Game()
	// itself, which used to do this inline) and empties it - shared with
	// Load(), which must drop the live session's own cached area state
	// before adopting a save's, or a load would otherwise resurrect
	// whatever the *current*, unsaved session left behind for any area
	// not being loaded right now instead of that save's own history.
	void _ClearAreaCache();

	// Fills fNPCs from a GAM's out-of-party table (a new game's BALDUR.GAM
	// or a save), skipping anyone already in the party.
	void _LoadNPCs(GamResource* gam);
	void _ClearNPCs();
	void _LoadStartingNPCs();
	// A character as a save (or BALDUR.GAM) describes it: the CRE's own
	// files, with the saved CRE state (if any - taken over and released
	// here) on top.
	Actor* _RestoreActor(const gam_party_member& member, CREResource* savedCre);
	gam_party_member _GamMember(Actor* actor, const res_ref& areaName) const;

	std::map<std::string, std::string> fTokens;
	std::vector<journal_entry> fJournal;
	std::map<std::string, bool> fAreaMapVisibility;


	// Loot window state - see OpenContainerWindow().
	class Object* fLootSource;
	Actor* fLooter;
	int32 fLootLeftRow;
	TargetMode fTargetMode;
	// What the action bar shows: the class row, or a page listing the
	// shown character's memorized spells / usable items (paged by
	// fActionBarPageIndex).
	enum ActionPage { PAGE_ROW, PAGE_SPELLS, PAGE_INNATES, PAGE_ITEMS };
	// Casts `spell` / uses the item in `slot` for `actor`, asking for a
	// target first unless it only affects its user.
	void _PickBarEntry(Actor* actor, const res_ref& name, int32 slot, bool spell);
	ActionPage fActionBarPage;
	uint32 fActionBarPageIndex;
	// The spell or item slot picked on the bar, waiting for its target.
	int32 fPendingItemSlot;
	// Quick spell slot (0-2) the spell page is choosing a spell for, or -1.
	int32 fAssignQuickSpell;
	res_ref fPendingSpell;
	int32 fLootRightRow;
	// HUD windows hidden for as long as the loot window is up, so
	// CloseContainerWindow() shows back exactly those.
	std::vector<uint16> fLootHiddenWindows;
	void _UpdateContainerWindow();

	// Store window state - see OpenStoreWindow(). fStores owns every Store
	// loaded this session, so a store keeps what was sold to it (and what
	// was bought out of it) when reopened; not written to savegames.
	std::map<std::string, class Store*> fStores;
	class Store* fStore;
	class Actor* fStoreCustomer;
	std::set<uint32> fStoreSellSlots;
	int32 fStoreLeftRow;
	int32 fStoreRightRow;
	bool fStoreUnpause;
	// The quantity picker: which shelf item (-1 = closed), the amount
	// chosen so far and the most that can be picked.
	int32 fStoreAmountIndex;
	uint32 fStoreAmountValue;
	uint32 fStoreAmountMax;
	void _OpenStoreAmountWindow(size_t shelfIndex);
	void _CloseStoreAmountWindow(bool apply);
	void _UpdateStoreAmountWindow();
	void _UpdateStoreWindow();
	// The store's page tabs: which page is showing (a store_page), the
	// action each of the bar's four tab buttons stands for, and the
	// Identify page's own selection/scroll.
	int32 fStorePage;
	std::vector<int32> fStoreTabs;
	std::set<uint32> fStoreIdentifySlots;
	int32 fStoreIdentifyRow;
	void _ShowStorePage(int32 page);
	void _SetupStoreTabs();
	void _UpdateStoreShopPage();
	void _UpdateStoreIdentifyPage();
	void _StoreIdentifySelected();
	void _StoreBuySelected();
	void _StoreSellSelected();
	void _ClearStores();

	// Party index whose sheet the Inventory / Record screens show.
	uint16 fShownCharacter;

	void _RunExecFile(GameConsole* console);
	// Re-populates the Inventory / Record screens (whichever are open)
	// after fShownCharacter changes.
	void _RefreshCharacterScreens();
	ScreenManager* fScreens;
};
