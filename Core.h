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


struct object_node;
class Actor;
class Door;
class Object;
class Region;
class RoomContainer;
class IDSResource;
class Script;
class TLKResource;
class Core {
public:
	static Core* Get();
	static bool Initialize(const char* path);
	static void Destroy();

	void TogglePause();

	void RegisterObject(Object* object);
	void UnregisterObject(Object* object);

	uint32 Game() const;

	void EnteredArea(RoomContainer* area, Script* script);

	void SetVariable(const char* name, int32 value);
	int32 GetVariable(const char* name);

	Object* GetObject(Object* source, object_node* node) const;
	Object* GetObject(const char* name) const;
	Object* GetObject(uint16 globalEnum) const;
	Object* GetObject(const Region* region) const;
	Object* GetNearestEnemyOf(const Object* object) const;

	void PlayMovie(const char* name);
	void DisplayMessage(uint32 strRef);

	void SetRoomScript(Script* script);

	void NewScriptRound();
	void CheckScripts();
	void UpdateLogic(bool scripts);

	// Actions/Triggers
	bool See(const Object* source, const Object* target) const;
	int Distance(const Object* source, const Object* target) const;

	void Open(Object* actor, Door* target);
	void Close(Object* actor, Door* target);

	void RandomFly(Actor* actor);
	void FlyToPoint(Actor* actor, IE::point, uint32 time);
	void RandomWalk(Actor* actor);

	static int32 RandomNumber(int32 start, int32 end);

	int32 GetObjectList(std::list<Reference<Object> >& objects) const;

private:
	// TODO: Remove this
	friend class Object;

	game fGame;
	RoomContainer* fCurrentRoom;
	
	Reference<Actor> fActiveActor;
	std::list<Reference<Object> > fObjects;
	
	std::map<std::string, uint32> fVariables;
	Script *fRoomScript;
	std::map<std::string, Script*> fScripts;

	uint32 fLastScriptRoundTime;
	uint16 fNextObjectNumber;

	bool fPaused;

	void _PrintObjects() const;
	void _RemoveStaleObjects();

	Core();
	~Core();
};



#endif // __CORE_H
