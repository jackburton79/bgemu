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


class Actor;
class Room;
class IDSResource;
class Script;
class ScriptContext;
class TLKResource;
class Core {
public:
	static Core* Get();

	game Game() const;

	void EnterArea(const char *name);
	Room *CurrentArea() const;

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	void PlayMovie(const char* name);

	void AddScript(Script* script);
	//void AddActorScript(const char* name, Script* script);
	//void RemoveActorScript(const char* name);

	void CheckScripts();
	void UpdateLogic();

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
