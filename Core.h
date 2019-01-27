#ifndef __CORE_H
#define __CORE_H

#include "Actor.h"
#include "IETypes.h"
#include "Reference.h"

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


struct actor_description {
	uint16 global_enum;
	res_ref area;
};

struct object_node;
class Actor;
class Door;
class Object;
class Region;
class RoomBase;
class IDSResource;
class Script;
class ScriptResults;
class TLKResource;

typedef std::list<Actor*> ActorsList;
typedef std::map<uint16, actor_description> ActorMap;

class Core {
public:
	static Core* Get();
	static bool Initialize(const char* path);
	static void Destroy();

	void TogglePause();
	bool IsPaused() const;

	void RegisterActor(Actor* object);
	void UnregisterActor(Actor* object);

	uint32 Game() const;

	bool LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName);
	bool LoadWorldMap();
	
	RoomBase* CurrentRoom();
	void AddActorToCurrentArea(Actor* actor);
	
	void EnteredArea(RoomBase* area);
	void ExitingArea(RoomBase* area);

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	Actor* GetObject(Actor* source, object_node* node) const;
	Actor* GetObject(const char* name) const;
	Actor* GetObject(uint16 globalEnum) const;
	Actor* GetObject(const Region* region) const;
	Actor* GetNearestEnemyOf(const Actor* object) const;

	ScriptResults* RoundResults();
	ScriptResults* LastRoundResults();
	
	void PlayMovie(const char* name);
	void DisplayMessage(uint32 strRef);

	void SetRoomScript(Script* script);

	void CheckScripts();
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

	int32 GetObjectList(ActorsList& objects) const;

private:
	static void _InitGameTimers();
	void _PrintObjects() const;
	void _RemoveStaleObjects();
	void _NewRound();
	
	Core();
	~Core();

	game fGame;
	RoomBase* fCurrentRoom;
	
	Actor* fActiveActor;
	
	ActorsList fActiveActors;
	ActorMap fActors;
	
	std::map<std::string, uint32> fVariables;
	Script *fRoomScript;
	std::map<std::string, Script*> fScripts;

	uint32 fLastScriptRoundTime;
	uint16 fNextObjectNumber;

	ScriptResults* fCurrentRoundResults;
	ScriptResults* fLastRoundResults;
	
	bool fPaused;
};



#endif // __CORE_H
