#ifndef __CORE_H
#define __CORE_H

#include "IETypes.h"

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
class Object;
class Room;
class IDSResource;
class Script;
class TLKResource;
class Core {
public:
	static Core* Get();
	static bool Initialize(const char* path);
	static void Destroy();

	uint32 Game() const;

	uint32 Time() const;
	uint32 GameTime() const;

	void EnteredArea(Room* area, Script* script);

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	void RegisterObject(Object* object);
	void UnregisterObject(Object* object);

	Object* GetObject(Object* source, object_node* node);
	Object* GetObject(const char* name);
	Object* GetNearestEnemyOf(const Object* object) const;

	void PlayMovie(const char* name);

	void SetRoomScript(Script* script);

	void NewScriptRound();
	void CheckScripts();
	void UpdateLogic(bool scripts);

	// Actions/Triggers
	bool See(const Object* source, const Object* target) const;
	int Distance(const Object* source, const Object* target) const;

	void RandomFly(Actor* actor);
	void FlyToPoint(Actor* actor, IE::point, uint32 time);
	void RandomWalk(Actor* actor);

private:
	game fGame;
	Room* fCurrentRoom;
	Actor* fActiveActor;

	std::list<Object*> fObjects;
	std::map<std::string, uint32> fVariables;
	Script *fRoomScript;
	std::map<std::string, Script*> fScripts;
	//std::map<std::string, ScriptContext*> fScriptContextes;

	uint32 fLastScriptRoundTime;

	Core();
	~Core();
};



#endif // __CORE_H
