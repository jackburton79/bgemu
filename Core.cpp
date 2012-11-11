#include "Core.h"

#include "Actor.h"
#include "CreResource.h"
#include "Door.h"
#include "IDSResource.h"
#include "MveResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"

#include <sys/time.h>
#include <stdlib.h>
#include <vector>

#include <SDL.h>


static Core* sCore = NULL;

//const static uint32 kRoundDuration = 6000; // 6 second. Actually this is the


Core::Core()
	:
	fCurrentRoom(NULL),
	fActiveActor(NULL),
	fRoomScript(NULL),
	fLastScriptRoundTime(0),
	fGame(GAME_BALDURSGATE)
{
	srand(time(NULL));
}


Core::~Core()
{
	delete fCurrentRoom;
}


/* static */
Core*
Core::Get()
{
	return sCore;
}


bool
Core::Initialize()
{
	if (sCore != NULL)
		return true;

	try {
		sCore = new Core();
	} catch (...) {
		return false;
	}

	return true;
}


void
Core::Destroy()
{
	delete sCore;
}


game
Core::Game() const
{
	return fGame;
}


uint32
Core::Time() const
{
	struct timeval now;
	gettimeofday(&now, 0);
	return now.tv_usec;
}


uint32
Core::GameTime() const
{
	return SDL_GetTicks();
}


void
Core::EnteredArea(GameMap* area, Script* script)
{
	// TODO: Move this elsewhere
	fCurrentRoom = area;
	SetRoomScript(script);

	// The area script
	if (fRoomScript != NULL) {
		fRoomScript->Execute();
	}
}


GameMap*
Core::CurrentArea() const
{
	return fCurrentRoom;
}


void
Core::SetVariable(const char* name, int32 value)
{
	printf("SetVariable(%s, %d) (old value %d)\n", name, value, fVariables[name]);
	fVariables[name] = value;
}


int32
Core::GetVariable(const char* name)
{
	return fVariables[name];
}


Object*
Core::GetObject(Object* source, object_node* node)
{
	node->Print();

	const char* identifier = ObjectsIDS()->ValueFor(node->identifiers);
	if (identifier != NULL && !strcasecmp(identifier, "MYSELF")) {
		std::cout << "Found object (" << source->Name() << ")";
		std::cout << "(source))!!!" << std::endl;
		return source;
	}

	if (node->name[0] != '\0')
		return GetObject(node->name);

	std::vector<Actor*> actorList = Actor::List();
	std::vector<Actor*>::iterator i;
	for (i = actorList.begin(); i != actorList.end(); i++) {
		if ((*i)->MatchNode(node)) {
			std::cout << "object matched (" << (*i)->Name();
			std::cout << ")!!!" << std::endl;
			return *i;
		}
	}
	return NULL;
}


Object*
Core::GetObject(const char* name)
{
	// TODO: Check also doors, triggers, etc ?!?
	std::vector<Actor*> actorList = Actor::List();
	std::vector<Actor*>::iterator i;
	for (i = actorList.begin(); i != actorList.end(); i++) {
		if (!strcmp(name, (*i)->Name())) {
			std::cout << "object matched (" << name;
			std::cout << ")!!!" << std::endl;
			return *i;
		}
	}
	return NULL;
}


void
Core::PlayMovie(const char* name)
{
	MVEResource* resource = gResManager->GetMVE(name);
	if (resource != NULL) {
		resource->Play();
		gResManager->ReleaseResource(resource);
	}
}


void
Core::SetRoomScript(Script* script)
{
	fRoomScript = script;
}


void
Core::CheckScripts()
{
}


void
Core::UpdateLogic()
{
	//return;
	// TODO: Fix/Improve
	std::vector<Actor*>::iterator i;
	//if (SDL_GetTicks() > fLastScriptRoundTime + kRoundDuration / 6) {
		//fLastScriptRoundTime = SDL_GetTicks();
		for (i = Actor::List().begin(); i != Actor::List().end(); i++) {
			Actor *actor  = *i;
			actor->Update();
		}
	//}

	fActiveActor = NULL;
}




bool
Core::See(Object* source, Object* object)
{
	// TODO: improve
	if (object == NULL)
		return false;

	std::cout << source->Name() << " SEE ";
	std::cout << object->Name() << " ?" << std::endl;

	return object->IsVisible();
}


void
Core::RandomFly(Actor* actor)
{
	int16 randomX = (rand() % 200) - 100;
	int16 randomY = (rand() % 200) - 100;
	IE::point destination = actor->Position();
	destination.x += randomX;
	destination.y += randomY;
	FlyToPoint(actor, destination, 1);
}


void
Core::FlyToPoint(Actor* actor, IE::point point, uint32 time)
{
	// TODO:
	actor->SetFlying(true);
	if (rect_contains(fCurrentRoom->AreaRect(), point))
		actor->SetDestination(point);
}


void
Core::RandomWalk(Actor* actor)
{
	int16 randomX = (rand() % 30) - 15;
	int16 randomY = (rand() % 30) - 15;

	IE::point destination = actor->Position();
	destination.x += randomX;
	destination.y += randomY;
	actor->SetFlying(false);
	if (rect_contains(fCurrentRoom->AreaRect(), destination))
		actor->SetDestination(destination);
}

