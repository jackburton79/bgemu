#pragma once

#include "Actor.h"
#include "IETypes.h"
#include "Variables.h"

#include <map>
#include <string>

struct action_params;
struct trigger_params;

enum class game {
	GAME_BALDURSGATE,
	GAME_BALDURSGATE2,
	GAME_TORMENT
};


struct object_params;
class Actor;
class Container;
class DialogHandler;
class Door;
class Object;
class Region;
class RoomBase;
class IDSResource;
class Script;
class TLKResource;

typedef std::map<uint16, Object*> ObjectsList;
typedef std::vector<Actor*> ActorsList;
typedef std::vector<Container*> ContainersList;
typedef std::vector<Region*> RegionsList;
typedef std::vector<Door*> DoorsList;

class Core {
public:
	static Core* Get();
	static bool Initialize(const char* path);
	static void Destroy();

	void TogglePause();
	bool IsPaused() const;

	void RegisterObject(Object* object);
	void UnregisterObject(Object* object);

	game Game() const;

	bool LoadArea(const res_ref areaName, std::string longName,
					std::string entranceName);
	// direction: the edge the party left the current area through (see
	// SearchMap::EdgeDirection()'s 0=North/1=East/2=South/3=West
	// numbering) so WorldMap can reveal that area's AREA_VISIBLE_FROM_
	// ADJACENT-flagged neighbors on that side; -1 (the default) for a
	// manual open (HUD button/hotkey) - reveals nothing new, same as
	// real IE.
	bool LoadWorldMap(int direction = -1);

	// Closes the world map and makes the area the player opened it from
	// current again, with its actors/scripts/state exactly as left (see
	// LoadWorldMap()'s own comment - that area was backgrounded, not
	// unloaded). A no-op (returns false) if the current room isn't the
	// world map, or there's no such area (e.g. the world map was reached
	// with no area loaded yet).
	bool ReturnFromWorldMap();

	// Returns true if we are displaying the world map
	bool IsWorldMap() const;

	// Unloads and releases the current room
	void UnloadCurrentRoom();

	// Records an area change to apply once it's safe to do so
	void RequestAreaChange(const res_ref& areaName,
					const std::string& longName,
					const std::string& entranceName,
					Actor* clearActionsFor = NULL);

	// Same deferral as RequestAreaChange() above
	void RequestWorldMapLoad(int direction = -1);

	// True once RequestAreaChange()/RequestWorldMapLoad() has been
	// called but the actual switch hasn't run yet (still waiting for
	// UpdateLogic()'s own cutsceneActorBusy gate). Object::
	// ExecuteActions() checks this to stop popping further queued
	// actions for the rest of this tick once it's set - see that
	// call site's own comment for why.
	bool HasPendingTransition() const;

	RoomBase* CurrentRoom();

	void ClearAllActions();

	void StartCutsceneMode();
	void EndCutsceneMode();
	bool CutsceneMode() const;

	void StartCutscene(const res_ref& scriptName);

	Object* CutsceneActor() const;
	void SetCutsceneActor(Object* object);

	Actor* DialogInitiator() const;

	DialogHandler* Dialog();

	void PlaySound(const res_ref& soundRefName);
	// The last sound PlaySound() was asked for (played or not), for tests.
	std::string LastSoundPlayed() const;

	Variables& Vars();

	// Party-wide gold pool - modeled as a "GOLD" GLOBAL variable rather
	// than dedicated state (see TAKEPARTYGOLD/GIVEPARTYGOLD's own
	// comment in scripting/Actions.cpp), but funneled through here so
	// every caller (those actions, gold-item pickup in Actor::AddItem(),
	// the Inventory screen's gold label) agrees on the same key and the
	// same never-goes-negative clamp instead of each touching Vars()
	// directly.
	int32 PartyGold() const;
	void AddPartyGold(int32 amount);

	Region* RegionAtPoint(const IE::point& point);

	void PlayMovie(const char* name);

	void UpdateLogic(bool scripts);

	// Actions/Triggers
	void Open(Object* actor, Door* target);
	void Close(Object* actor, Door* target);

	uint32 ScriptRound() const;

	static int32 RandomNumber(int32 start, int32 end);
	static int32 RollDice(int32 count, int32 sides, int32 bonus);
	static ::Script* ExtractScript(const res_ref& resName);

private:

	static void _InitGameTimers();
	void _PrintObjects() const;
	void _NewRound();

	Core();
	~Core();

	game fGame;
	RoomBase* fCurrentRoom;
	// The area LoadWorldMap() backgrounded (not unloaded) so
	// ReturnFromWorldMap() can hand it straight back - see both methods'
	// own comments. NULL except while the world map is fCurrentRoom.
	RoomBase* fPreviousRoom;

	Variables fVariables;

	uint32 fLastScriptRoundTime;
	uint16 fNextObjectNumber;

	uint32 fCurrentRoundNumber;

	bool fPaused;
	bool fCutsceneMode;
	::Script* fCutsceneScript;
	bool fDialogMode;
	Object* fCutsceneActor;

	bool fPendingAreaChange;
	res_ref fPendingAreaName;
	std::string fPendingLongName;
	std::string fLastSound;
	std::string fPendingEntranceName;
	Actor* fPendingAreaChangeActor;
	bool fPendingWorldMapLoad;
	int fPendingWorldMapDirection;
};

