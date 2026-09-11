#include "Core.h"

#include "Actor.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Door.h"
#include "Game.h"
#include "GameTimer.h"
#include "Log.h"
#include "MoviePlayer.h"
#include "MveResource.h"
#include "ResManager.h"
#include "Script.h"
#include "SoundEngine.h"
#include "WAVResource.h"
#include "WorldMap.h"

#include <limits.h>
#include <stdlib.h>


static Core* sCore = NULL;

//const static uint32 kRoundDuration = 6000; // 6 second. Actually this is the

Core::Core()
	:
	fGame(game::GAME_BALDURSGATE2),
	fCurrentRoom(NULL),
	fLastScriptRoundTime(0),
	fNextObjectNumber(0),
	fCurrentRoundNumber(0),
	fPaused(false),
	fCutsceneMode(false),
	fCutsceneScript(NULL),
	fDialogMode(false),
	fCutsceneActor(NULL),
	fPendingAreaChange(false),
	fPendingAreaChangeActor(NULL),
	fPendingWorldMapLoad(false),
	fHasExtendedOrientations(false)
{
	srand(time(NULL));
}


Core::~Core()
{
	delete fCutsceneScript;
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
		sCore->fGame = game::GAME_BALDURSGATE2;
		sCore->fHasExtendedOrientations = true;
		std::cout << "Baldur's Gate 2" << std::endl;
	} else {
		sCore->fGame = game::GAME_BALDURSGATE;
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
	if (sCore == NULL)
		return;

	Timer::TearDown();

	std::cout << "Core::Destroy()" << std::endl;
	// Normally already NULL by now - Game::Loop() calls
	// UnloadCurrentRoom() itself
	sCore->UnloadCurrentRoom();
	ResourceManager::Destroy();
	delete sCore;
}


void
Core::UnloadCurrentRoom()
{
	if (fCurrentRoom != NULL) {
		fCurrentRoom->Unload();
		fCurrentRoom->Release();
		fCurrentRoom = NULL;
	}
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


game
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
	UnloadCurrentRoom();
	try {
		// No Acquire() here: the object already starts at refcount 1
		// from its own constructor
		fCurrentRoom = new AreaRoom(areaName, longName.c_str(), entranceName.c_str());
	} catch (std::exception& e) {
		std::cerr << Log::Red << e.what() << std::endl;
		return false;
	}

	EnteredArea(fCurrentRoom);
	return true;
}


void
Core::RequestAreaChange(const res_ref& areaName, const std::string& longName,
	const std::string& entranceName, Actor* clearActionsFor)
{
	fPendingAreaChange = true;
	fPendingAreaName = areaName;
	fPendingLongName = longName;
	fPendingEntranceName = entranceName;
	fPendingAreaChangeActor = clearActionsFor;
}


void
Core::RequestWorldMapLoad()
{
	fPendingWorldMapLoad = true;
}


bool
Core::LoadWorldMap()
{
	// TODO:
	UnloadCurrentRoom();
	try {
		// No Acquire() here - see LoadArea()'s own comment
		fCurrentRoom = new WorldMap();
	} catch (std::exception& e) {
		std::cerr << Log::Red << "Core::LoadWorldMap: " << e.what() << std::endl;
		return false;
	}

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

	if (fCutsceneScript != NULL) {
		// A cutscene was already running: shouldn't normally happen, but
		// don't leak it if it does.
		std::cerr << Log::Red << "Core::StartCutscene(): a cutscene script "
			"is already running, replacing it" << std::endl;
		delete fCutsceneScript;
	}

	fCutsceneScript = ExtractScript(scriptName);
	if (fCutsceneScript != NULL) {
		fCutsceneActor = NULL;
		StartCutsceneMode();
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
	if (soundRefName.CString()[0] == '\0')
		return;

	WAVResource* wav = gResManager->GetWAV(soundRefName);
	if (wav == NULL)
		return;

	std::vector<uint8> samples;
	uint16 channels;
	uint16 bitsPerSample;
	uint32 sampleRate;
	if (wav->DecodePCM(samples, channels, bitsPerSample, sampleRate)
			&& SoundEngine::Get() != NULL) {
		SoundEngine::Get()->PlaySample(samples.data(), (uint32)samples.size(),
			channels, bitsPerSample, sampleRate);
	}

	gResManager->ReleaseResource(wav);
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
		MoviePlayer player;
		player.Play(resource);
		gResManager->ReleaseResource(resource);
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

	if (!Game::Get()->InDialogMode()) {
		// Cutscene mode suppresses script (re-)evaluation uniformly for
		// every object in the area
		bool runScripts = executeScripts && !fCutsceneMode;
		// AreaRoom::Update() calls Update() for every object
		fCurrentRoom->Update(runScripts);

		// Apply any area change an action requested during the Update()
		// call just above now that it's actually safe to do so
		bool cutsceneActorBusy = fCutsceneActor != NULL
			&& !fCutsceneActor->IsActionListEmpty();
		if (fPendingAreaChange && !cutsceneActorBusy) {
			fPendingAreaChange = false;
			if (fPendingAreaChangeActor != NULL) {
				fPendingAreaChangeActor->ClearActionList();
				fPendingAreaChangeActor = NULL;
			}
			LoadArea(fPendingAreaName, fPendingLongName, fPendingEntranceName);
			return;
		}

		if (fPendingWorldMapLoad && !cutsceneActorBusy) {
			fPendingWorldMapLoad = false;
			LoadWorldMap();
			return;
		}

		if (fCutsceneScript != NULL && fCutsceneScript->ExecuteCutscene()) {
			// Every block has been *queued* onto its target actor(s) - but
			// not yet executed.Wait for it to actually be
			// done before lifting cutscene mode.
			// TODO: I think we should not do this here, and just wait for the EndCutsceneMode op
			Object* actor = fCutsceneActor;
			if (actor == NULL || actor->IsActionListEmpty()) {
				delete fCutsceneScript;
				fCutsceneScript = NULL;
				fCutsceneActor = NULL;
				EndCutsceneMode();
			}
		}

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


uint32
Core::ScriptRound() const
{
	return fCurrentRoundNumber;
}


/* static */
int32
Core::RandomNumber(int32 start, int32 end)
{
	if (start == end)
		return start;
	return start + rand() % (end - start + 1);
}


/* static */
int32
Core::RollDice(int32 count, int32 sides, int32 bonus)
{
	int32 total = bonus;
	for (int32 i = 0; i < count; i++)
		total += RandomNumber(1, sides);
	return total;
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
Core::_InitGameTimers()
{
	Timer::Set("ANIMATIONS", 100);
	Timer::Set("ANIMATEDTILES", 120);
}


void
Core::_NewRound()
{
	fCurrentRoundNumber++;
}
