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
class Action;
class Container;
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
	
	uint32 Game() const;

	bool LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName);
	bool LoadWorldMap();
	
	RoomBase* CurrentRoom();

	void AddActorToCurrentArea(Actor* actor);
		
	void EnteredArea(RoomBase* area);
	void ExitingArea(RoomBase* area);

	void AddGlobalAction(Action* action);
	void StartCutsceneMode();
	void EndCutsceneMode();
	bool CutsceneMode() const;
	void StartCutscene(const res_ref& scriptName);
	void SetCutsceneActor(Object* object);

	bool InDialogMode() const;
	void SetDialogMode(bool value);

	void PlayAnimation(const res_ref& name, const IE::point where);
	void PlayEffect(const res_ref& name, const IE::point where);
	void PlaySound(const res_ref& soundRefName);

	Variables& Vars();

	void SetActiveActor(Actor* actor);
	Actor* ActiveActor();
	
	Object* GetObject(const char* name) const;
	Object* GetObject(uint16 globalEnum) const;
	Actor* GetObjectFromNode(object_node* node) const;
	Actor* GetObject(const Region* region) const;
	Actor* GetNearestEnemyOf(const Actor* object) const;
	Actor* GetNearestEnemyOfType(const Actor* object, int ieClass) const;

	Region* RegionAtPoint(const IE::point& point);
	
	void PlayMovie(const char* name);

	// TODO: Move away from here, don't belong here
	void DisplayMessage(const char* actor, const char* text);

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
	static ::Script* ExtractScript(const res_ref& resName);

	int32 GetActorsList(ActorsList& objects) const;

private:
	static void _InitGameTimers();
	void _PrintObjects() const;
	void _CleanDestroyedObjects();
	void _NewRound();
	
	Core();
	~Core();

	game fGame;
	RoomBase* fCurrentRoom;
	
	Actor* fActiveActor;

	ObjectsList fObjects;
	
	ActorsList fActors;
	ContainersList fContainers;
	RegionsList fRegions;
	DoorsList fDoors;

	Variables fVariables;
	std::map<std::string, Script*> fScripts;

	uint32 fLastScriptRoundTime;
	uint16 fNextObjectNumber;

	uint32 fCurrentRoundNumber;

	bool fPaused;
	bool fCutsceneMode;
	bool fDialogMode;
	Object* fCutsceneActor;
};



#endif // __CORE_H
