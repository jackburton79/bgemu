#include "Core.h"

#include "Action.h"
#include "Actor.h"
#include "AreaRoom.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "GUI.h"
#include "IDSResource.h"
#include "MveResource.h"
#include "Party.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "RoundResults.h"
#include "Script.h"
#include "TextArea.h"
#include "Timer.h"
#include "TLKResource.h"
#include "Window.h"
#include "WorldMap.h"

#include <algorithm>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <vector>


static Core* sCore = NULL;

//const static uint32 kRoundDuration = 6000; // 6 second. Actually this is the

Core::Core()
	:
	fGame(GAME_BALDURSGATE2),
	fCurrentRoom(NULL),
	fActiveActor(NULL),
	fRoomScript(NULL),
	fLastScriptRoundTime(0),
	fNextObjectNumber(0),
	fCurrentRoundResults(NULL),
	fLastRoundResults(NULL),
	fPaused(false)
{
	srand(time(NULL));
}


Core::~Core()
{
	std::cout << "Core::~Core() returned" << std::endl;
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

	std::cout << "Core::Initialize()" << std::endl;
	if (path == NULL || path[0] == 0) {
		std::cerr << "Core::Initialize(): No path supplied" << std::endl;
		return false;
	}
		
	try {
		sCore = new Core();
	} catch (...) {
		return false;
	}

	std::cout << "\t-> Initializing Resource Manager..." << std::endl;
	if (!ResourceManager::Initialize(path))
		return false;

	// Detect game
	// TODO: Find a better/safer way
	std::cout << "Core::Initialize()" << std::endl;
	std::cout << "\t-> Detecting game... ";
	std::flush(std::cout);
	std::vector<std::string> stringList;
	if (gResManager->GetResourceList(stringList, "CSJON", RES_CRE) == 1) {
		sCore->fGame = GAME_BALDURSGATE2;
		std::cout << "Baldur's Gate 2" << std::endl;
	} else {
		sCore->fGame = GAME_BALDURSGATE;
		std::cout << "Baldur's Gate" << std::endl;
	}

	Timer::Initialize();

	_InitGameTimers();

	sCore->fCurrentRoundResults = new ScriptResults();
	sCore->fLastRoundResults = new ScriptResults();
	
	std::cout << "Core::Initialize(): OK! " << std::endl;
	return true;
}


void
Core::Destroy()
{
	std::cout << "Core::Destroy()" << std::endl;
	delete sCore->fCurrentRoom;
	sCore->fCurrentRoom = NULL;
	ResourceManager::Destroy();
	delete sCore;
}


void
Core::TogglePause()
{
	fPaused = !fPaused;
}


bool
Core::IsPaused() const
{
	return fPaused;
}


uint32
Core::Game() const
{
	return fGame;
}


RoomContainer*
Core::CurrentRoom()
{
	return fCurrentRoom;
}


bool
Core::LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName)
{
	delete fCurrentRoom;
	fCurrentRoom = NULL;
	try {
		fCurrentRoom = new AreaRoom(areaName, longName, entranceName);
	} catch (...) {
		return false;
	}
	return true;
}


bool
Core::LoadWorldMap()
{
	// TODO:
	delete fCurrentRoom;
	fCurrentRoom = NULL;
	try {
		fCurrentRoom = new WorldMap();
	} catch (...) {
		return false;
	}
	return true;
}


void
Core::EnteredArea(RoomContainer* area, Script* script)
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

	_PrintObjects();
}


void
Core::ExitingArea(RoomContainer* area)
{
	std::list<Reference<Object> >::const_iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		UnregisterObject(i->Target());
	}
	fObjects.clear();
}


void
Core::SetVariable(const char* name, int32 value)
{
	//std::cout << "SetVariable(" << name << ", " << value;
	//std::cout << " (old value: " << fVariables[name] << ")";
	//std::cout << std::endl;
	fVariables[name] = value;
}


int32
Core::GetVariable(const char* name)
{
	//std::cout << "GetVariable(" << name << "): " << fVariables[name];
	//std::cout << std::endl;
	return fVariables[name];
}


void
Core::RegisterObject(Object* object)
{
	fObjects.push_back(object);
	if (Actor* actor = dynamic_cast<Actor*>(object)) {
		if (actor->IsNew())
			actor->CRE()->SetGlobalActorEnum(fNextObjectNumber++);
	}
}


// Called when an object is no longer in the active area
// Also deletes the object
void
Core::UnregisterObject(Object* object)
{
	// TODO: Save the object state
	object->Release();
}


Object*
Core::GetObject(Object* source, object_node* node) const
{
	// TODO: Move into object_node::Print()
	/*std::cout << "Core::GetObject(";
	std::cout << "source: " << source->Name() << ", ";
	std::cout << "node: ( ";
	if (node->name[0])
		std::cout << node->name;
	if (node->general)
		std::cout << ", " << IDTable::GeneralAt(node->general);
	if (node->classs)
		std::cout << ", " << IDTable::ClassAt(node->classs);
	if (node->specific)
		std::cout << ", " << IDTable::SpecificAt(node->specific);
	if (node->ea)
		std::cout << ", " << IDTable::EnemyAllyAt(node->ea);
	if (node->gender)
		std::cout << ", " << IDTable::GenderAt(node->gender);
	if (node->race)
		std::cout << ", " << IDTable::RaceAt(node->race);
	if (node->alignment)
		std::cout << ", " << IDTable::AlignmentAt(node->alignment);
	std::cout << ") ) -> " ;
*/
	if (node->name[0] != '\0')
		return GetObject(node->name);

	// If there are any identifiers, use those to get the object
	if (node->identifiers[0] != 0) {
		Object* target = NULL;
		for (int32 id = 0; id < 5; id++) {
			const int identifier = node->identifiers[id];
			if (identifier == 0)
				break;
			//std::cout << IDTable::ObjectAt(identifier) << ", ";
			target = source->ResolveIdentifier(identifier);
			source = target;
		}
		// TODO: Filter using wildcards in node
	/*	std::cout << "returned ";
		if (target != NULL)
			std::cout << target->Name() << std::endl;
		else
			std::cout << "NONE" << std::endl;*/
		return target;
	}


	// Otherwise use the other parameters
	// TODO: Simplify, merge code.
	std::list<Reference<Object> >::const_iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		if (i->Target()->MatchNode(node)) {
			//std::cout << "returned " << (*i)->Name() << std::endl;
			//(*i)->Print();
			return i->Target();
		}
	}

	//std::cout << "returned NONE" << std::endl;
	return NULL;
}


Object*
Core::GetObject(const char* name) const
{
	std::list<Reference<Object> >::const_iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		if (!strcmp(name, i->Target()->Name())) {
			return i->Target();
		}
	}

	return NULL;
}


Object*
Core::GetObject(uint16 globalEnum) const
{
	std::list<Reference<Object> >::const_iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		Actor* actor = dynamic_cast<Actor*>(i->Target());
		if (actor != NULL && actor->CRE()->GlobalActorEnum() == globalEnum)
			return i->Target();
	}

	return NULL;
}


Object*
Core::GetObject(const Region* region) const
{
	// TODO: Only returns the first object!
	std::list<Reference<Object> >::const_iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		Actor* actor = dynamic_cast<Actor*>(i->Target());
		if (actor == NULL)
			continue;
		if (region->Contains(actor->Position()))
			return actor;
	}

	return NULL;
}


Object*
Core::GetNearestEnemyOf(const Object* object) const
{
	std::list<Reference<Object> >::const_iterator i;
	int minDistance = INT_MAX;
	Actor* nearest = NULL;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		Actor* actor = dynamic_cast<Actor*>(i->Target());
		if (actor == NULL)
			continue;
		if (i->Target() != object && i->Target()->IsEnemyOf(object)) {
			int distance = Distance(object, i->Target());
			if (distance < minDistance) {
				minDistance = distance;
				nearest = actor;
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
	// TODO: Move away from Core ? this adds too many
	// dependencies
	try {
		std::string dialogString = IDTable::GetDialog(strRef);
		std::cout << dialogString << std::endl;
		if (Window* window = GUI::Get()->GetWindow(4)) {
			TextArea *textArea = dynamic_cast<TextArea*>(
										window->GetControlByID(3));
			if (textArea != NULL)
				textArea->SetText(dialogString.c_str());
		}
	} catch (...) {
		//TODO: handle exception
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
Core::UpdateLogic(bool executeScripts)
{
	if (fPaused)
		return;

	GameTimer::UpdateGameTime();
	Timer* timer = Timer::Get("ANIMATIONS");
	if (timer->Expired())
		timer->Rearm();
	timer = Timer::Get("ANIMATEDTILES");
	if (timer->Expired())
		timer->Rearm();

	// TODO: Not nice, should stop the scripts in some other way
	if (strcmp(fCurrentRoom->Name(), "WORLDMAP") == 0)
		return;

	// TODO: Fix/Improve
	std::list<Reference<Object> >::iterator i;
	for (i = fObjects.begin(); i != fObjects.end(); i++) {
		Object* object = i->Target();
		object->Update(executeScripts);
	}

	fActiveActor = NULL;

	_NewRound();
	
	//_RemoveStaleObjects();
}


bool
Core::See(const Object* source, const Object* object) const
{
	// TODO: improve
	if (object == NULL)
		return false;

	const Actor* sourceActor = dynamic_cast<const Actor*>(source);
	if (sourceActor == NULL)
		return false;

	std::cout << source->Name() << " SEE ";
	std::cout << object->Name() << " ?" << std::endl;

	return sourceActor->HasSeen(object);
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
Core::Open(Object* actor, Door* door)
{
	if (!door->Opened())
		door->Open(actor);
}


void
Core::Close(Object* actor, Door* door)
{
	if (door->Opened())
		door->Close(actor);
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
	int16 randomX = (rand() % 100) - 50;
	int16 randomY = (rand() % 100) - 50;

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	FlyTo* flyTo = new FlyTo(actor, destination, time);
	actor->AddAction(flyTo);
}


void
Core::RandomWalk(Actor* actor)
{
	int16 randomX = (rand() % 100) - 50;
	int16 randomY = (rand() % 100) - 50;

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	WalkTo* walkTo = new WalkTo(actor, destination);
	actor->AddAction(walkTo);
}


/* static */
int32
Core::RandomNumber(int32 start, int32 end)
{
	return start + rand() % (end - start);
}


int32
Core::GetObjectList(std::list<Reference<Object> > & objects) const
{
	objects = fObjects;
	return objects.size();
}


ScriptResults*
Core::RoundResults()
{
	return fCurrentRoundResults;
}


ScriptResults*
Core::LastRoundResults()
{
	return fLastRoundResults;
}


/*
const std::list<Object*>&
Core::Objects() const
{
	return fObjects;
}
*/

void
Core::_PrintObjects() const
{
	for (std::list<Reference<Object> >::const_iterator i = fObjects.begin();
											i != fObjects.end(); i++) {
		i->Target()->Print();
	}
}


void
Core::_InitGameTimers()
{
	Timer::Set("ANIMATIONS", 100);
	Timer::Set("ANIMATEDTILES", 120);
}


void
Core::_RemoveStaleObjects()
{
	std::list<Reference<Object> >::iterator i = fObjects.begin();
	while (i != fObjects.end()) {
		if (i->Target()->IsStale()) {
			//if (Actor* actor = dynamic_cast<Actor*>(i->Target())) {
				//->ActorExitedArea(actor);
			//}
			i = fObjects.erase(i);
		} else
			i++;
	}
}


void
Core::_NewRound()
{
	std::swap(fCurrentRoundResults, fLastRoundResults);
	fCurrentRoundResults->Clear();
}
