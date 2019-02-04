#include "Core.h"

#include "Action.h"
#include "Actor.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Container.h"
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
	fLastScriptRoundTime(0),
	fNextObjectNumber(0),
	fCurrentRoundNumber(0),
	fCurrentRoundResults(NULL),
	fLastRoundResults(NULL),
	fPaused(false),
	fCutsceneMode(false)
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


RoomBase*
Core::CurrentRoom()
{
	return fCurrentRoom;
}


void
Core::AddActorToCurrentArea(Actor* actor)
{
	actor->SetArea(CurrentRoom()->Name());
	RegisterActor(actor);
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

	EnteredArea(fCurrentRoom);
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

	//EnteredArea(fCurrentRoom);
	return true;
}


void
Core::EnteredArea(RoomBase* area)
{
	// Executes the area script (once)
	//area->Update(true);
	
	// then clear the script
	//area->ClearScripts();
	
	//_PrintObjects();
}


void
Core::ExitingArea(RoomBase* area)
{
	area->ClearScripts();
	
	ActorsList::const_iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		UnregisterActor(*i);
	}
	fActiveActors.clear();
	
	// containers and regions are owned by the AreaRoom class
	// TODO: Fix and / or clear ownership
	fContainers.clear();
	fRegions.clear();
}


Variables&
Core::Vars()
{
	return fVariables;
}


void
Core::SetActiveActor(Actor* actor)
{
	fActiveActor = actor;
	if (fActiveActor)
		std::cout << "SetActiveActor(" << actor->Name() << ")" << std::endl;
}


Actor*
Core::ActiveActor()
{
	if (fActiveActor)
		std::cout << "ActiveActor is " << fActiveActor->Name() << std::endl;
	return fActiveActor;
}


void
Core::AddGlobalAction(Action* action)
{
	fActions.push_back(action);
}


void
Core::StartCutsceneMode()
{
	fCutsceneMode = true;
}


bool
Core::CutsceneMode() const
{
	return fCutsceneMode;
}


void
Core::StartCutscene(const res_ref& scriptName)
{
	::Script* script = ExtractScript(scriptName);
	// TODO: not nice. but it will be changed in the cutscene script
	script->SetSender(fCurrentRoom);
	bool continuing = false;
	if (script != NULL) {
		script->Execute(continuing);
		delete script;
	}	
}


void
Core::RegisterActor(Actor* actor)
{
	fActiveActors.push_back(actor);
	if (actor->IsNew())
		actor->CRE()->SetGlobalActorEnum(fNextObjectNumber++);
}


// Called when an object is no longer in the active area
// Also deletes the object
void
Core::UnregisterActor(Actor* actor)
{
	// TODO: Save the object state
	actor->Release();
}


void
Core::RegisterContainer(Container* container)
{
	fContainers.push_back(container);
}


void
Core::RegisterRegion(Region* region)
{
	fRegions.push_back(region);
}


Object*
Core::GetObject(const char* name) const
{
	ActorsList::const_iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		if (!strcasecmp(name, (*i)->Name())) {
			return *i;
		}
	}

	RegionsList::const_iterator r;
	for (r = fRegions.begin(); r != fRegions.end(); r++) {
		if (!strcasecmp(name, (*r)->Name())) {
			return *r;
		}
	}
	return NULL;
}


Actor*
Core::GetObject(uint16 globalEnum) const
{
	ActorsList::const_iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		Actor* actor = *i;
		if (actor != NULL && actor->CRE()->GlobalActorEnum() == globalEnum)
			return actor;
	}

	return NULL;
}


Actor*
Core::GetObject(object_node* node) const
{
	// TODO: Simplify, merge code.
	ActorsList::const_iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		if ((*i)->MatchNode(node)) {
			//std::cout << "returned " << (*i)->Name() << std::endl;
			//(*i)->Print();
			return *i;
		}
	}
	return NULL;
}


Actor*
Core::GetObject(const Region* region) const
{
	// TODO: Only returns the first object!
	ActorsList::const_iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		Actor* actor = *i;
		if (actor == NULL)
			continue;
		if (region->Contains(actor->Position()))
			return actor;
	}

	return NULL;
}


Actor*
Core::GetNearestEnemyOf(const Actor* object) const
{
	ActorsList::const_iterator i;
	int minDistance = INT_MAX;
	Actor* nearest = NULL;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		Actor* actor = *i;
		if (actor == NULL)
			continue;
		if (actor != object && actor->IsEnemyOf(object)) {
			int distance = Distance(object, actor);
			if (distance < minDistance) {
				minDistance = distance;
				nearest = actor;
			}
		}
	}
	if (nearest != NULL) {
		/*std::cout << "Nearest Enemy of " << object->Name();
		std::cout << " is " << nearest->Name() << std::endl;*/
	}
	return nearest;
}


Actor*
Core::GetNearestEnemyOfType(const Actor* object, int ieClass) const
{
	ActorsList::const_iterator i;
	int minDistance = INT_MAX;
	Actor* nearest = NULL;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		Actor* actor = *i;
		if (actor == NULL)
			continue;
		if (actor != object && actor->IsEnemyOf(object) && actor->IsClass(ieClass)) {
			int distance = Distance(object, actor);
			if (distance < minDistance) {
				minDistance = distance;
				nearest = actor;
			}
		}
	}
	if (nearest != NULL) {
		std::cout << "Nearest Enemy of " << object->Name();
		std::cout << " (type " << ieClass << ")";
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
Core::UpdateLogic(bool executeScripts)
{
	if (fPaused)
		return;

	//GameTimer::PrintTime();
	GameTimer::UpdateGameTime();
		
	// TODO: Not nice, should stop the scripts in some other way
	if (strcmp(fCurrentRoom->Name(), "WORLDMAP") == 0)
		return;

	Timer* timer = Timer::Get("ANIMATIONS");
	if (timer->Expired())
		timer->Rearm();
	timer = Timer::Get("ANIMATEDTILES");
	if (timer->Expired())
		timer->Rearm();

	_HandleGlobalActions();
	
	// The room
	fCurrentRoom->Update(executeScripts);

	// TODO: Fix/Improve
	ActorsList::iterator i;
	for (i = fActiveActors.begin(); i != fActiveActors.end(); i++) {
		Actor* actor = *i;
		//SetActiveActor(actor);
		actor->Update(executeScripts);
	}

	SetActiveActor(NULL);
#if 0
	ContainersList::iterator c;
	for (c = fContainers.begin(); c != fContainers.end(); c++) {
		Object* object = *c;
		object->Update(executeScripts);
	}
	
	RegionsList::iterator r;
	for (r = fRegions.begin(); r != fRegions.end(); r++) {
		Object* object = *r;
		object->Update(executeScripts);
	}
#endif
	_CleanDestroyedObjects();
	
	_NewRound();
}


bool
Core::See(const Actor* sourceActor, const Object* object) const
{
	// TODO: improve
	if (object == NULL)
		return false;

	std::cout << sourceActor->Name() << " SEE ";
	std::cout << object->Name() << " ?" << std::endl;

	return sourceActor->HasSeen(object);
}


int
Core::Distance(const Object* a, const Object* b) const
{
	const Actor* actor = dynamic_cast<const Actor*>(a);
	if (actor == NULL) {
		std::cerr << "Distance: requested for non-actor object!" << std::endl;
		return 0;
	}
		
	const IE::point positionA = actor->Position();
	const IE::point positionB = b->NearestPoint(positionA);

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
	int16 randomX = RandomNumber(-100, 100);
	int16 randomY = RandomNumber(-100, 100);

	FlyToPoint(actor, offset_point(actor->Position(), randomX,
							randomY), 1);
}


void
Core::FlyToPoint(Actor* actor, IE::point point, uint32 time)
{
	int16 randomX = RandomNumber(-50, 50);
	int16 randomY = RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	FlyTo* flyTo = new FlyTo(actor, destination, time);
	actor->AddAction(flyTo);
}


void
Core::RandomWalk(Actor* actor)
{
	int16 randomX = RandomNumber(-50, 50);
	int16 randomY = RandomNumber(-50, 50);

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

/* static */
::Script*
Core::ExtractScript(const res_ref& resName)
{
	::Script* script = NULL;
	BCSResource* scriptResource = gResManager->GetBCS(resName);	
	if (scriptResource != NULL) {
		script = scriptResource->GetScript();
		gResManager->ReleaseResource(scriptResource);
	}
	return script;
}


int32
Core::GetActorsList(ActorsList& objects) const
{
	objects = fActiveActors;
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


void
Core::_PrintObjects() const
{
	for (ActorsList::const_iterator i = fActiveActors.begin();
											i != fActiveActors.end(); i++) {
		(*i)->Print();
	}
}


void
Core::_InitGameTimers()
{
	Timer::Set("ANIMATIONS", 100);
	Timer::Set("ANIMATEDTILES", 120);
}


void
Core::_CleanDestroyedObjects()
{
	// TODO: Remove objects pointers from
	// RoundResults!!!
	ActorsList::iterator i = fActiveActors.begin();
	while (i != fActiveActors.end()) {
		Object* object = *i;
		if (object->ToBeDestroyed()) {
			if (Actor* actor = dynamic_cast<Actor*>(object)) {
				std::cout << "Destroy actor " << actor->Name() << std::endl;
				UnregisterActor(actor);
			}
			i = fActiveActors.erase(i);
		} else
			i++;
	}
}


void
Core::_CheckIfInsideRegion(Actor* actor)
{
	Region* previousRegion = actor->CurrentRegion();
	RegionsList::iterator r;
	for (r = fRegions.begin(); r != fRegions.end(); r++) {
		Region* region = *r;		
		if (rect_contains(region->Frame(), actor->Position())) {
			actor->SetRegion(region);
			break; 
		}	
	}
	// TODO: exited region
	if (previousRegion != actor->CurrentRegion())
		fCurrentRoundResults->SetActorEnteredRegion(actor, actor->CurrentRegion());	
}


void
Core::_NewRound()
{
	fCurrentRoundNumber++;
	std::swap(fCurrentRoundResults, fLastRoundResults);
	fCurrentRoundResults->Clear();
	//std::cout << "******* ROUND " << fCurrentRoundNumber << "********" << std::endl;
}


void
Core::_HandleGlobalActions()
{
	std::vector<Action*>::iterator i = fActions.begin();
	if (i != fActions.end()) {
		Action& action = **i;
		action();
		if (action.Completed())
			fActions.erase(i);
	}
}
