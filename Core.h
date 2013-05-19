#ifndef __CORE_H
#define __CORE_H

#include "IETypes.h"

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
class ScriptContext;
class TLKResource;
class Core {
public:
	static Core* Get();
	static bool Initialize();
	static void Destroy();

	// TODO: These are static for now, since currently ResourceManager
	// has to be instantiated before Core, but needs these two methods
	static game Game();
	static void SetGame(game g);

	uint32 Time() const;
	uint32 GameTime() const;

	void EnteredArea(Room* area, Script* script);
	Room *CurrentArea() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	Object* GetObject(Object* source, object_node* node);
	Object* GetObject(const char* name);
	Object* GetNearestEnemyOf(const Object* object) const;

	void PlayMovie(const char* name);

	void SetRoomScript(Script* script);

	void NewScriptRound();
	void CheckScripts();
	void UpdateLogic();

	// Actions/Triggers
	bool See(const Object* source, const Object* target) const;
	int Distance(const Object* source, const Object* target) const;

	void RandomFly(Actor* actor);
	void FlyToPoint(Actor* actor, IE::point, uint32 time);
	void RandomWalk(Actor* actor);

private:
	Room* fCurrentRoom;
	Actor* fActiveActor;

	std::map<std::string, uint32> fVariables;
	Script *fRoomScript;
	std::map<std::string, Script*> fScripts;
	std::map<std::string, ScriptContext*> fScriptContextes;

	uint32 fLastScriptRoundTime;

	static game sGame;

	Core();
	~Core();
};



#endif // __CORE_H
