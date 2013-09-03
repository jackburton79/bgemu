#include "Core.h"

#include "Actor.h"
#include "CreResource.h"
#include "Door.h"
#include "GUI.h"
#include "IDSResource.h"
#include "MveResource.h"
#include "Party.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "TextArea.h"
#include "TLKResource.h"
#include "Window.h"

#include <sys/time.h>
#include <stdlib.h>
#include <algorithm>
#include <ctype.h>
#include <vector>

#include <SDL.h>


static Core* sCore = NULL;

//const static uint32 kRoundDuration = 6000; // 6 second. Actually this is the

Core::Core()
	:
	fGame(GAME_BALDURSGATE2),
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
Core::Initialize(const char* path)
{
	if (sCore != NULL)
		return true;

	try {
		sCore = new Core();
	} catch (...) {
		return false;
	}

	if (!gResManager->Initialize(path))
		return false;

	// Detect game
	// TODO: Find a better/safer way
	std::cout << "Detecting game... ";
	std::vector<std::string> stringList;
	if (gResManager->GetResourceList(stringList, "CSJON", RES_CRE) == 1) {
		sCore->fGame = GAME_BALDURSGATE2;
		std::cout << "GAME_BALDURSGATE2" << std::endl;
	} else {
		sCore->fGame = GAME_BALDURSGATE;
		std::cout << "GAME_BALDURSGATE" << std::endl;
	}

	std::cout << std::endl;


	// TODO: Move this elsewhere.
	// This should be filled by the player selection

	try {
		IE::point point = { 20, 20 };
		Party* party = Party::Get();
		if (sCore->fGame == GAME_BALDURSGATE)
			party->AddActor(new Actor("AJANTI", point, 0));
		else
			party->AddActor(new Actor("CERND10", point, 0));
	} catch (...) {

	}
	return true;
}


void
Core::Destroy()
{
	delete sCore;
}


uint32
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
Core::EnteredArea(Room* area, Script* script)
{
	// TODO: Move this elsewhere
	fCurrentRoom = area;

	area->SetScript(script);
	SetRoomScript(script);

	// The area script
	if (fRoomScript != NULL) {
		fRoomScript->Execute();
	}

	area->SetScript(NULL);
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


void
Core::RegisterObject(Object* object)
{
	fObjects.push_back(object);
}


void
Core::UnregisterObject(Object* object)
{
	fObjects.remove(object);
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
	std::list<Object*>::iterator i;
	std::cout << "Matching..." << std::endl;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
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
	std::list<Object*>::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
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
Core::DisplayMessage(uint32 strRef)
{
	// TODO: Move away from Script ? this adds too many
	// dependencies
	TLKEntry* entry = Dialogs()->EntryAt(strRef);
	if (Window* tmp = GUI::Get()->GetWindow(4)) {
		TextArea *textArea = dynamic_cast<TextArea*>(tmp->GetControlByID(3));
		if (textArea != NULL)
			textArea->SetText(entry->string);
	}
	delete entry;
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

	// TODO: Not nice, should be stop the scripts in some other way
	if (strcmp(Room::Get()->AreaName().CString(), "WORLDMAP") == 0)
		return;

	// TODO: Fix/Improve
	std::list<Object*>::iterator i;
	//if (SDL_GetTicks() > fLastScriptRoundTime + kRoundDuration / 6) {
		//fLastScriptRoundTime = SDL_GetTicks();
		for (i = fObjects.begin(); i != fObjects.end(); i++) {
			(*i)->Update(executeScripts);
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
	const IE::point positionA = a->Position();
	const IE::point positionB = b->Position();

	IE::point invalidPoint = { -1, -1 };
	if (positionA == invalidPoint && positionB == invalidPoint)
		return 100; // TODO: ???

	return positionA - positionB;
}


void
Core::RandomFly(Actor* actor)
{
	int16 randomX = (rand() % 200) - 100;
	int16 randomY = (rand() % 200) - 100;

	FlyToPoint(actor, offset_point(actor->Position(), randomX,
							randomY), 1);
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
	int16 randomX = (rand() % 100) - 50;
	int16 randomY = (rand() % 100) - 50;

	actor->SetFlying(false);
	actor->SetDestination(offset_point(actor->Position(), randomX, randomY));
}

