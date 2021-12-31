#include "Core.h"

#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Container.h"
#include "CreResource.h"
#include "Door.h"
#include "Effect.h"
#include "Game.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Log.h"
#include "MveResource.h"
#include "Party.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
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
	fLastScriptRoundTime(0),
	fNextObjectNumber(0),
	fCurrentRoundNumber(0),
	fPaused(false),
	fCutsceneMode(false),
	fDialogMode(false),
	fCutsceneActor(NULL),
	fHasExtendedOrientations(false)
{
	srand(time(NULL));
}


Core::~Core()
{
	//std::cout << "Core::~Core() returned" << std::endl;
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
		std::cerr << Log::Red << "Core::Initialize(): No path supplied" << std::endl;
		return false;
	}
		
	try {
		sCore = new Core();
	} catch (...) {
		return false;
	}

	if (!ResourceManager::Initialize(path))
		return false;

	// Detect game
	// TODO: Find a better/safer way
	std::cout << "Core: Detecting game... ";
	std::flush(std::cout);
	if (gResManager->ResourceExists("CSJON", RES_CRE)) {
		sCore->fGame = GAME_BALDURSGATE2;
		sCore->fHasExtendedOrientations = true;
		std::cout << "Baldur's Gate 2" << std::endl;
	} else {
		sCore->fGame = GAME_BALDURSGATE;
		sCore->fHasExtendedOrientations = false;
		std::cout << "Baldur's Gate" << std::endl;
	}

	Timer::Initialize();

	_InitGameTimers();

	return true;
}


void
Core::Destroy()
{
	std::cout << "Core::Destroy()" << std::endl;
	if (sCore->fCurrentRoom != NULL) {
		sCore->fCurrentRoom->Release();
		sCore->fCurrentRoom = NULL;
	}
	ResourceManager::Destroy();
	delete sCore;
}


bool
Core::HasExtendedOrientations() const
{
	return fHasExtendedOrientations;
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


bool
Core::LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName)
{
	if (fCurrentRoom != NULL) {
		fCurrentRoom->Release();
		fCurrentRoom = NULL;
	}
	try {
		fCurrentRoom = new AreaRoom(areaName, longName, entranceName);
		fCurrentRoom->Acquire();
	} catch (std::exception& e) {
		std::cerr << Log::Red << e.what() << std::endl;
		return false;
	}

	EnteredArea(fCurrentRoom);
	return true;
}


bool
Core::LoadWorldMap()
{
	// TODO:
	if (fCurrentRoom != NULL) {
		fCurrentRoom->Release();
		fCurrentRoom = NULL;
	}
	try {
		fCurrentRoom = new WorldMap();
		fCurrentRoom->Acquire();
	} catch (std::exception& e) {
		std::cerr << Log::Red << "Core::LoadWorldMap: " << e.what() << std::endl;
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

}


Variables&
Core::Vars()
{
	return fVariables;
}


void
Core::ClearAllActions()
{
#if 0
	for (ActorsList::iterator a = fActors.begin(); a != fActors.end(); a++)
		(*a)->ClearActionList();
#endif
}


void
Core::StartCutsceneMode()
{
	fCutsceneMode = true;
}


void
Core::EndCutsceneMode()
{
	fCutsceneMode = false;
}


bool
Core::CutsceneMode() const
{
	return fCutsceneMode;
}


void
Core::StartCutscene(const res_ref& scriptName)
{
	std::cout << "Core::StartCutscene():" << scriptName.CString() << std::endl;
	::Script* script = ExtractScript(scriptName);
	bool continuing = false;
	if (script != NULL) {
		//std::cout << "Executing script" << std::endl;
		// TODO: not nice. but it will be changed in the cutscene script
		//script->SetSender(fCurrentRoom);
		script->Execute(continuing);
		
		// TODO: We cannot delete the script, since actions are parsing it after we return.
		// We are leaking the it now
		//delete script;
	}	
}


Object*
Core::CutsceneActor() const
{
	return fCutsceneActor;
}


void
Core::SetCutsceneActor(Object* actor)
{
	fCutsceneActor = actor;
}


void
Core::PlayAnimation(const res_ref& name, const IE::point where)
{
	AreaRoom* area = dynamic_cast<AreaRoom*>(CurrentRoom());
	if (area == NULL)
		return;

	Animation* animation = new Animation(name.CString(), 0, false, where);
	area->AddAnimation(animation);
	// TODO: Delete when done
	// at the moment we are leaking the animation
}


void
Core::PlayEffect(const res_ref& name, const IE::point where)
{
	AreaRoom* area = dynamic_cast<AreaRoom*>(CurrentRoom());
	if (area == NULL)
		return;

	Effect* effect = new Effect(name, where);
	area->AddEffect(effect);
}


void
Core::PlaySound(const res_ref& soundRefName)
{
	//
}


void
Core::RegisterObject(Object* object)
{
	//std::cout << "Core::RegisterObject(" << object->Name() << ")" << std::endl;
	// TODO: Not nice.
	// introduce a "type" field in Object, and maybe put all objects into the same list ?
	/*if (Actor* actor = dynamic_cast<Actor*>(object))
		fActors.push_back(actor);
	else if (Door* door = dynamic_cast<Door*>(object))
		fDoors.push_back(door);
	else if (Region* region = dynamic_cast<Region*>(object))
		fRegions.push_back(region);
	else if (Container* container = dynamic_cast<Container*>(object))
		fContainers.push_back(container);
*/
	//if (object->IsNew())
		object->SetGlobalID(fNextObjectNumber++);

	// TODO: Check if already registered
	//fObjects[object->GlobalID()] = object;
}


void
Core::UnregisterObject(Object* object)
{
	// TODO: Save the object state
	// TODO: Implement
	// TODO: Remove from list
	//fObjects.erase(object->GlobalID());
	//object->Release();
	//if (Actor* actor = dynamic_cast<Actor*>(object))
		//actor->Release();
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
Core::DisplayMessage(Object* object, const char* text)
{
	// TODO: Move away from Core ? this adds too many
	// dependencies

	// Show text on screen
	if (object != NULL) {
		GFX::rect frame = rect_to_gfx_rect(object->Frame());
		Core::Get()->CurrentRoom()->ConvertFromArea(frame);
		GUI::Get()->DisplayStringCentered(text, frame.x, frame.y, 5000);
	}

	// Write text in TextArea
	TextArea* textArea = GUI::Get()->GetMessagesTextArea();
	if (textArea != NULL) {
		std::string fullText;
		if (object != NULL)
			fullText.append(object->Name()).append(": ");
		fullText.append(text);
		textArea->AddText(fullText.c_str());
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

	/*Timer* timer = Timer::Get("ANIMATIONS");
	if (timer->Expired())
		timer->Rearm();
	timer = Timer::Get("ANIMATEDTILES");
	if (timer->Expired())
		timer->Rearm();
	*/
	if (!Game::Get()->InDialogMode()) {
		fCurrentRoom->Update(executeScripts);
		// TODO: Fix/Improve

		// THIS WAS MOVED TO AREAROOM
		/*ObjectsList::iterator i;
		for (i = fObjects.begin(); i != fObjects.end(); i++) {
			Object* object = i->second;
			//SetActiveActor(actor);
			object->Update(executeScripts);
		}*/
		//SetActiveActor(NULL);
		_CleanDestroyedObjects();

		_NewRound();
	}
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
	/*int16 randomX = RandomNumber(-50, 50);
	int16 randomY = RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	FlyTo* flyTo = new FlyTo(actor, destination, time);
	actor->AddAction(flyTo);*/
}


void
Core::RandomWalk(Actor* actor)
{
	/*int16 randomX = RandomNumber(-50, 50);
	int16 randomY = RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	WalkTo* walkTo = new WalkTo(actor, destination);
	actor->AddAction(walkTo);*/
}


/* static */
int32
Core::RandomNumber(int32 start, int32 end)
{
	if (start == end)
		return start;
	return start + rand() % (end - start);
}


/* static */
::Script*
Core::ExtractScript(const res_ref& resName)
{
	if (resName == "" || resName == "None")
		return NULL;

	::Script* script = NULL;
	BCSResource* scriptResource = gResManager->GetBCS(resName);	
	if (scriptResource != NULL) {
		script = scriptResource->GetScript();
		gResManager->ReleaseResource(scriptResource);
	}
	return script;
}


void
Core::_PrintObjects() const
{
/*	for (ActorsList::const_iterator i = fActors.begin();
											i != fActors.end(); i++) {
		(*i)->Print();
	}*/
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
/*	ActorsList::iterator i = fActors.begin();
	while (i != fActors.end()) {
		Object* object = *i;
		if (object->ToBeDestroyed()) {
			if (object == fCutsceneActor) {
				// TODO: is this correct ?
				//GUI::Get()->Show();
				fCutsceneMode = false;
				fCutsceneActor = NULL;
				//return;	
			}
			std::cout << "Destroy actor " << object->Name() << std::endl;			
			object->ClearActionList();
			if (Actor* actor = dynamic_cast<Actor*>(object)) {
				UnregisterObject(actor);
			}
			i = fActors.erase(i);
		} else
			i++;
	}*/
}


void
Core::_NewRound()
{
	fCurrentRoundNumber++;
	//std::cout << "******* ROUND " << fCurrentRoundNumber << "********" << std::endl;
}
