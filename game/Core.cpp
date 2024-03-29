#include "Core.h"

#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Container.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Log.h"
#include "MveResource.h"
#include "Party.h"
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
Core::LoadArea(const res_ref areaName, std::string longName,
					std::string entranceName)
{
	if (fCurrentRoom != NULL) {
		fCurrentRoom->Release();
		fCurrentRoom = NULL;
	}
	try {
		fCurrentRoom = new AreaRoom(areaName, longName.c_str(), entranceName.c_str());
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
	if (Game::Get()->InDialogMode())
		Game::Get()->TerminateDialog();
	::Script* script = ExtractScript(scriptName);
	if (script != NULL) {
		script->ExecuteCutscene();
		
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
Core::PlaySound(const res_ref& soundRefName)
{
	//
}


void
Core::RegisterObject(Object* object)
{
	// TODO: Check if already registered
	if (object->IsNew()) {
		object->SetGlobalID(fNextObjectNumber++);
		//std::cout << "RegisterObject " << object->Name() << ": " << object->GlobalID() << std::endl;
	}
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
		// AreaRoom::Update() calls Update() for every object
		fCurrentRoom->Update(executeScripts);

		_NewRound();
	}
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
Core::_NewRound()
{
	fCurrentRoundNumber++;
	//std::cout << "******* ROUND " << fCurrentRoundNumber << "********" << std::endl;
}
