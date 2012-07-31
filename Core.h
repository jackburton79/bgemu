#ifndef __WORLD_H
#define __WORLD_H

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

	game Game() const;

	void EnteredArea(Room* area, Script* script);
	Room *CurrentArea() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	Object* GetObject(Object* source, object_node* node);

	void PlayMovie(const char* name);

	void SetRoomScript(Script* script);
	//void AddActorScript(const char* name, Script* script);
	//void RemoveActorScript(const char* name);

	void CheckScripts();
	void UpdateLogic();

	bool See(Object* source, Object* target);

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

	game fGame;

	Core();
	~Core();
};




TLKResource* Dialogs();
IDSResource* GeneralIDS();
IDSResource* AnimateIDS();
IDSResource* AniSndIDS();
IDSResource* GendersIDS();
IDSResource* RacesIDS();
IDSResource* ClassesIDS();
IDSResource* SpecificIDS();
IDSResource* TriggerIDS();
IDSResource* ActionIDS();
IDSResource* ObjectsIDS();
IDSResource* EAIDS();


#endif // __WORLD_H
