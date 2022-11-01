#ifndef __CORE_H
#define __CORE_H

#include "Actor.h"
#include "IETypes.h"
#include "Reference.h"
#include "Variables.h"

#include <list>
#include <map>
#include <string>

struct action_params;
struct trigger_params;

enum game {
	GAME_BALDURSGATE,
	GAME_BALDURSGATE2,
	GAME_TORMENT
};


struct object_params;
class Actor;
class Action;
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
	
	uint32 Game() const;

	bool LoadArea(const res_ref areaName, std::string longName,
					std::string entranceName);
	bool LoadWorldMap();
	
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

	// TODO: Move away from here, don't belong here
	void DisplayMessage(Object* object, const char* text);

	void UpdateLogic(bool scripts);

	// Actions/Triggers
	void Open(Object* actor, Door* target);
	void Close(Object* actor, Door* target);

	void RandomFly(Actor* actor);
	void FlyToPoint(Actor* actor, IE::point, uint32 time);
	void RandomWalk(Actor* actor);

	static int32 RandomNumber(int32 start, int32 end);
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
	bool fDialogMode;
	Object* fCutsceneActor;

	// Engine features
	bool fHasExtendedOrientations;
};



#endif // __CORE_H
