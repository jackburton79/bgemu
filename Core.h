#ifndef __CORE_H
#define __CORE_H

#include "Actor.h"
#include "IETypes.h"
#include "Reference.h"
#include "Variables.h"

#include <list>
#include <map>
#include <string>

struct action_node;
struct node;
struct trigger_node;

enum game {
	GAME_BALDURSGATE,
	GAME_BALDURSGATE2,
	GAME_TORMENT
};


struct object_node;
class Actor;
class Container;
class Door;
class Object;
class Region;
class RoomBase;
class IDSResource;
class Script;
class ScriptResults;
class TLKResource;

typedef std::vector<Actor*> ActorsList;
typedef std::vector<Container*> ContainersList;
typedef std::vector<Region*> RegionsList;

class Core {
public:
	static Core* Get();
	static bool Initialize(const char* path);
	static void Destroy();

	void TogglePause();
	bool IsPaused() const;

	void RegisterActor(Actor* object);
	void UnregisterActor(Actor* object);
	
	void RegisterContainer(Container* container);
	void RegisterRegion(Region* region);

	uint32 Game() const;

	bool LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName);
	bool LoadWorldMap();
	
	RoomBase* CurrentRoom();

	void AddActorToCurrentArea(Actor* actor);
		
	void EnteredArea(RoomBase* area);
	void ExitingArea(RoomBase* area);

	void StartCutsceneMode();
	bool CutsceneMode() const;

	Variables& Vars();

	Actor* GetObject(const char* name) const;
	Actor* GetObject(uint16 globalEnum) const;
	Actor* GetObject(object_node* node) const;
	Actor* GetObject(const Region* region) const;
	Actor* GetNearestEnemyOf(const Actor* object) const;
	Actor* GetNearestEnemyOfType(const Actor* object, int ieClass) const;

	ScriptResults* RoundResults();
	ScriptResults* LastRoundResults();
	
	void PlayMovie(const char* name);
	void DisplayMessage(uint32 strRef);

	void UpdateLogic(bool scripts);

	// Actions/Triggers
	bool See(const Actor* source, const Object* target) const;
	int Distance(const Object* source, const Object* target) const;

	void Open(Object* actor, Door* target);
	void Close(Object* actor, Door* target);

	void RandomFly(Actor* actor);
	void FlyToPoint(Actor* actor, IE::point, uint32 time);
	void RandomWalk(Actor* actor);

	static int32 RandomNumber(int32 start, int32 end);

	int32 GetActorsList(ActorsList& objects) const;

private:
	static void _InitGameTimers();
	void _PrintObjects() const;
	void _CheckIfInsideRegion(Actor* actor);
	void _RemoveStaleObjects();
	void _NewRound();
	
	Core();
	~Core();

	game fGame;
	RoomBase* fCurrentRoom;
	
	Actor* fActiveActor;
	
	ActorsList fActiveActors;
	ContainersList fContainers;
	RegionsList fRegions;
	
	Variables fVariables;
	std::map<std::string, Script*> fScripts;

	uint32 fLastScriptRoundTime;
	uint16 fNextObjectNumber;

	uint32 fCurrentRoundNumber;
	ScriptResults* fCurrentRoundResults;
	ScriptResults* fLastRoundResults;
	
	bool fPaused;
	bool fCutsceneMode;
};



#endif // __CORE_H
