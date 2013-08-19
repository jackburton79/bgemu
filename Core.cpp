#include "Core.h"

#include "Actor.h"
#include "CreResource.h"
#include "Door.h"
#include "IDSResource.h"
#include "MveResource.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"

#include <sys/time.h>
#include <stdlib.h>
#include <algorithm>
#include <ctype.h>
#include <vector>

#include <SDL.h>


static Core* sCore = NULL;

//const static uint32 kRoundDuration = 6000; // 6 second. Actually this is the

game Core::sGame = GAME_BALDURSGATE2;

Core::Core()
	:
	fCurrentRoom(NULL),
	fActiveActor(NULL),
	fRoomScript(NULL),
	fLastScriptRoundTime(0)
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
Core::Game()
{
	return sGame;
}


void
Core::SetGame(game g)
{
	sGame = g;
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
Core::EnteredArea(Room* area, Script* script)
{
	// TODO: Move this elsewhere
	fCurrentRoom = area;

	SetRoomScript(script);

	// The area script
	if (fRoomScript != NULL) {
		fRoomScript->Execute();
	}
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

	if (node->name[0] != '\0')
		return GetObject(node->name);

	// If there are any identifiers, use those to get the object
	if (node->identifiers[0] != 0) {
		Object* target = NULL;
		for (int32 id = 0; id < 5; id++) {
			const int identifier = node->identifiers[id];
			if (identifier == 0)
				break;
			target = source->ResolveIdentifier(identifier);
			source = target;
		}
		// TODO: Filter using wildcards in node
		return target;
	}


	// Otherwise use the other parameters
	// TODO: Simplify, merge code.
	std::vector<Actor*> actorList = Actor::List();
	std::vector<Actor*>::iterator i;
	std::cout << "Matching..." << std::endl;
	for (i = actorList.begin(); i != actorList.end(); i++) {
		if ((*i)->MatchNode(node)) {
			std::cout << "GetObject() returned " << std::endl;
			(*i)->Print();
			return *i;
		}
	}

	std::cout << "GetObject() returned NONE" << std::endl;
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
			std::cout << "GetObject() returned " << name;
			std::cout << std::endl;
			return *i;
		}
	}

	std::cout << "GetObject() returned NONE" << std::endl;
	return NULL;
}


Object*
Core::GetNearestEnemyOf(const Object* object) const
{
	std::vector<Actor*> actorList = Actor::List();
	std::vector<Actor*>::iterator i;
	int minDistance = 500000;
	Actor* nearest = NULL;
	for (i = actorList.begin(); i != actorList.end(); i++) {
		if ((*i) != object && (*i)->IsEnemyOf(object)) {
			int distance = Distance(object, *i);
			if (distance < minDistance) {
				minDistance = distance;
				nearest = *i;
			}
		}
	}
	if (nearest != NULL) {
		std::cout << "Nearest Enemy of " << object->Name();
		std::cout << " is " << nearest->Name() << std::endl;
	}
	return nearest;
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
Core::NewScriptRound()
{

}


void
Core::CheckScripts()
{
}


void
Core::UpdateLogic(bool executeScripts)
{
	// TODO: Should do that based on timer.
	//NewScriptRound();

	/*std::map<std::string, Door*>::iterator doorIterator;
	for (doorIterator = Door::List().begin();
			doorIterator != Door::List().end();
			doorIterator++) {
		(*doorIterator).second->Update();
	}*/
	//return;

	/*std::vector<Region*>::iterator regionIter;
	for (regionIter = Region::List().begin();
			regionIter != Region::List().end(); regionIter++) {
		(*regionIter)->Update();
	}
*/
	//return;
	// TODO: Fix/Improve
	std::vector<Actor*>::iterator i;
	//if (SDL_GetTicks() > fLastScriptRoundTime + kRoundDuration / 6) {
		//fLastScriptRoundTime = SDL_GetTicks();
		for (i = Actor::List().begin(); i != Actor::List().end(); i++) {
			Actor *actor  = *i;
			actor->Update(executeScripts);
		}
	//}

	fActiveActor = NULL;
}


bool
Core::See(const Object* source, const Object* object) const
{
	// TODO: improve
	if (object == NULL)
		return false;

	std::cout << source->Name() << " SEE ";
	std::cout << object->Name() << " ?" << std::endl;

	// TODO: Just a hack to make things go on
	if (object->IsVisible()) {
		//Object* wObject = const_cast<Object*>(source);
		const_cast<std::vector<Object*>&>(source->CurrentScriptRoundResults()
			->SeenList()).push_back(const_cast<Object*>(object));
	}
	return object->IsVisible();
}


int
Core::Distance(const Object* a, const Object* b) const
{
	const Actor* actorA = dynamic_cast<const Actor*>(a);
	const Actor* actorB = dynamic_cast<const Actor*>(b);

	if (actorA == NULL || actorB == NULL)
		return 100; // TODO: ???

	return actorA->Position() - actorB->Position();
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

	//Room* area = Core::CurrentArea();
	//std::cout << "New destination: " << std::dec << destination.x << " " << destination.y << std::endl;
	//std::cout << "area: " << area->AreaRect().w << " " << area->AreaRect().h << std::endl;
}


void
Core::FlyToPoint(Actor* actor, IE::point point, uint32 time)
{
	// TODO:
	actor->SetFlying(true);
	//if (rect_contains(fCurrentRoom->AreaRect(), point))
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
	//if (rect_contains(fCurrentRoom->AreaRect(), destination))
	actor->SetDestination(destination);
}

