#ifndef __CORE_H
#define __CORE_H

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

	bool HasExtendedOrientations() const;

	void TogglePause();
	bool IsPaused() const;

	void RegisterObject(Object* object);
	void UnregisterObject(Object* object);

	game Game() const;

	bool LoadArea(const res_ref areaName, std::string longName,
					std::string entranceName);
	bool LoadWorldMap();

	// Records an area change to apply once it's safe to do so (right
	// after UpdateLogic()'s AreaRoom::Update() call returns) instead of
	// loading immediately - LoadArea() destroys the current AreaRoom
	// (see its own GUI::Clear() call), which is only safe once nothing
	// on the call stack is still inside one of ITS OWN member functions.
	// Actions that trigger an area change (LEAVEAREALUA,
	// MOVEBETWEENAREASEFFECT) run from deep inside AreaRoom::Update()'s
	// own actor-update loop (Actor::Update() -> ... -> this action) -
	// calling LoadArea() straight from there was a real, reproduced
	// heap-use-after-free (AreaRoom::Update()'s "for (actor : fActors)"
	// loop reading fActors after the AreaRoom that owns it, and that
	// loop, got destroyed out from under it).
	// clearActionsFor (optional): also clears that actor's own action
	// list right before the deferred LoadArea() below runs - for a
	// caller triggered by the actor's own movement finally arriving
	// somewhere (see Actor::_UpdateRegions()), where the action that
	// walked it there is done and letting it (or anything queued
	// behind it) survive into the new area makes no sense. Deferred to
	// the same safe point as the area load itself, for the same
	// reason: RequestAreaChange() can be called from deep inside that
	// very actor's own action execution (e.g. a teleport action whose
	// handler still reads its own action_params after this call
	// returns) - clearing its action list synchronously here would
	// free that action out from under its own still-running handler.
	void RequestAreaChange(const res_ref& areaName,
					const std::string& longName,
					const std::string& entranceName,
					Actor* clearActionsFor = NULL);

	// Same deferral as RequestAreaChange() above, same reason: a party
	// member walking onto a wilderness map's "Worldmap exit" search-map
	// cell (see SearchMap::IsWorldmapExit()) requests this from
	// Actor::_UpdateRegions(), itself reachable from deep inside
	// AreaRoom::Update()'s own actor loop - LoadWorldMap() destroys the
	// current AreaRoom exactly like LoadArea() does.
	void RequestWorldMapLoad();

	RoomBase* CurrentRoom();

	void EnteredArea(RoomBase* area);
	void ExitingArea(RoomBase* area);

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

	Variables& Vars();

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
	std::string fPendingEntranceName;
	Actor* fPendingAreaChangeActor;
	bool fPendingWorldMapLoad;

	// Engine features
	bool fHasExtendedOrientations;
};



#endif // __CORE_H
