/*
 * Game.cpp
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#include "Game.h"

#include "2DAResource.h"
#include "Actor.h"
#include "Core.h"
#include "CreResource.h"
#include "Dialog.h"
#include "GameConsole.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Parsing.h"
#include "Party.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "TextArea.h"
#include "Timer.h"


#include <assert.h>
#include <stdio.h>


static uint32 sFrames = 0;
static uint32 sLastFrame = 0;
static uint32 sLastTime = 0;

static Game* sGame;

/* static */
Game*
Game::Get()
{
	if (sGame == NULL)
		sGame = new Game();
	return sGame;
}


Game::Game()
	:
	fDialog(NULL),
	fParty(NULL),
	fDelay(40),
	fTestMode(false)
{
}


Game::~Game()
{
	TerminateDialog();
	delete fParty;
}


void
DisplayClock(void *param)
{
	std::string clock = GameTimer::GameTimeString();
	GFX::rect frame = GraphicsEngine::Get()->ScreenFrame();
	GUI::Get()->DisplayString(clock.c_str(), frame.x + 10, frame.h - 50, 2500);
}


void
DisplayFrameRate(void* param)
{
	uint32 currentTime = SDL_GetTicks();
	GFX::rect frame = GraphicsEngine::Get()->ScreenFrame();
	char frameRate[32];
	uint32 numFrames = sFrames - sLastFrame;
	uint32 elapsedTime = currentTime - sLastTime;
	if (elapsedTime == 0)
		elapsedTime = 1;
	snprintf(frameRate, sizeof(frameRate), "FPS: %d", 1000 * numFrames / elapsedTime);
	GUI::Get()->DisplayString(frameRate, frame.x + 70, 20, 1000);
	sLastFrame = sFrames;
	sLastTime = currentTime;
}


void
Game::Loop(bool noNewGame, bool executeScripts)
{
	std::cout << "Game::Loop()" << std::endl;
	if (!GUI::Initialize(GraphicsEngine::Get()->ScreenFrame().w,
						 GraphicsEngine::Get()->ScreenFrame().h)) {
		throw std::runtime_error("Initializing GUI failed");
	}

	GFX::rect screenRect = GraphicsEngine::Get()->ScreenFrame();
	GUI* gui = GUI::Get();
	GFX::rect consoleRect(
			0,
			0,
			screenRect.w,
			screenRect.h);

	GameConsole* inputConsole = NULL;
#if 1
	std::cout << "Setting up console... ";
	std::flush(std::cout);
	inputConsole = new GameConsole(consoleRect, true);
	std::cout << "OK!" << std::endl;
#endif
	if (inputConsole != NULL)
		inputConsole->Initialize();
	std::cout << "OK!" << std::endl;

	if (TestMode()) {
		GUI::Get()->Load("GUITEST");
	} else {
		try {
			CreateParty();
		} catch (...) {
			throw std::runtime_error("Error creating player!");
		}
		if (noNewGame)
			Core::Get()->LoadWorldMap();
		else
			LoadStartingArea();
	}
	std::cout << "Game: Started game loop." << std::endl;
	SDL_Event event;
	bool quitting = false;

	int clockTimer = Timer::AddPeriodicTimer(8000, DisplayClock, NULL);
	int fpsTimer = Timer::AddPeriodicTimer(1000, DisplayFrameRate, NULL);

	while (!quitting) {
		uint32 startTicks = Timer::Ticks();
		while (SDL_PollEvent(&event) != 0) {
			RoomBase* room = Core::Get()->CurrentRoom();
			switch (event.type) {
				case SDL_USEREVENT: {
					// Used to handle timers
					void (*event_func)(void*) = (void (*)(void*))event.user.data1;
					event_func(event.user.data2);
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
					gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					gui->MouseMoved(event.motion.x, event.motion.y);
					break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						if (inputConsole != NULL)
							inputConsole->Toggle();
					} else if (inputConsole != NULL && inputConsole->IsActive()) {
						if (event.key.keysym.scancode < 0x80 && event.key.keysym.scancode > 0) {
							uint8 key = event.key.keysym.sym;
							if (event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
								key -= 32;
							if (inputConsole != NULL)
								inputConsole->HandleInput(key);
						}
					} else {
						switch (event.key.keysym.sym) {
							case SDLK_o:
								if (room != NULL)
									room->ToggleOverlays();
								break;
							case SDLK_d:
								if (inputConsole != NULL) {
									if (inputConsole->HasOutputRedirected())
										inputConsole->DisableRedirect();
									else
										inputConsole->EnableRedirect();
									break;
								}
							// TODO: Move to GUI class
							case SDLK_h:
								if (room != NULL)
									room->ToggleGUI();
								break;
							case SDLK_a:
								if (room != NULL)
									room->ToggleAnimations();
								break;
							case SDLK_p:
								if (room != NULL)
									room->TogglePolygons();
								break;
							case SDLK_w:
								Core::Get()->LoadWorldMap();
								break;
							case SDLK_s:
								if (room != NULL)
									room->ToggleSearchMap();
								break;
							case SDLK_n:
								ToggleDayNight();
								break;
							case SDLK_q:
								quitting = true;
								break;
							case SDLK_PLUS:
								if (--fDelay == 0)
									fDelay = 1;
								break;
							case SDLK_MINUS:
								fDelay++;
								break;
							case SDLK_1:
								GUI::Get()->ToggleWindow(1);
								break;
							case SDLK_2:
								GUI::Get()->ToggleWindow(2);
								break;
							case SDLK_3:
								GUI::Get()->ToggleWindow(3);
								break;
							case SDLK_4:
								GUI::Get()->ToggleWindow(4);
								break;
							case SDLK_SPACE:
								Core::Get()->TogglePause();
								break;
							default:
								break;
						}
					}
				}
				break;

				case SDL_QUIT:
					quitting = true;
					break;
				default:
					break;
			}
		}

		gui->Draw();

		if (inputConsole != NULL)
			inputConsole->Draw();
		if (!TestMode())
			Core::Get()->UpdateLogic(executeScripts);
		GraphicsEngine::Get()->Update();
		
		sFrames++;
		Timer::WaitSync(startTicks, fDelay);
	}

	Timer::RemovePeriodicTimer(clockTimer);
	Timer::RemovePeriodicTimer(fpsTimer);

	delete inputConsole;

	std::cout << "Game: Input loop stopped." << std::endl;

	GUI::Destroy();
}


void
Game::CreateParty()
{
	assert(fParty == NULL);
	// TODO: This should be filled by the player selection
	IE::point point = { 20, 20 };
	fParty = new ::Party();
	if (Core::Get()->Game() == GAME_BALDURSGATE)
		fParty->AddActor(new Actor("AJANTI", point, 0));
	else
		fParty->AddActor(new Actor("ANOMEN10", point, 0));
}


void
Game::InitiateDialog(Actor* actor, Actor* target)
{
	assert(fDialog == NULL);

	const res_ref dialogFile = actor->CRE()->DialogFile();
	if (dialogFile.name[0] == '\0'
			|| strcasecmp(dialogFile.CString(), "None") == 0) {
		std::cout << "EMPTY DIALOG FILE" << std::endl;
		return;
	}

	GUI::Get()->EnsureShowDialogArea();

	trigger_entry triggerEntry("LastTalkedToBy", actor);
	actor->AddTrigger(triggerEntry);
	std::cout << "initiates dialog with " << actor->LongName() << std::endl;
	std::cout << "Dialog file: " << dialogFile << std::endl;

	fDialog = new DialogHandler(actor, target, dialogFile);
}


bool
Game::InDialogMode() const
{
	return fDialog != NULL;
}


void
Game::TerminateDialog()
{
	if (InDialogMode()) {
		std::cout << fDialog->Actor()->Name() << " TerminateDialog()" << std::endl;
		fDialog->Actor()->IncrementNumTimesTalkedTo();
	}
	delete fDialog;
	fDialog = NULL;
}


DialogHandler*
Game::Dialog()
{
	return fDialog;
}


Party*
Game::Party()
{
	return fParty;
}


void
Game::LoadStartingArea()
{
	std::cout << "Load Starting Area...";
	TWODAResource* resource = gResManager->Get2DA("STARTARE");
	if (resource == NULL) {
		std::cout << "Failed!" << std::endl;
		return;
	}

	std::cout << "OK!" << std::endl;
	std::string startingArea = resource->ValueFor("START_AREA", "VALUE");
	IE::point viewPosition;
	viewPosition.x = resource->IntegerValueFor("START_XPOS", "VALUE");
	viewPosition.y = resource->IntegerValueFor("START_YPOS", "VALUE");

	std::cout << "Starting area: " << startingArea << std::endl;
	std::cout << "Starting position: " << viewPosition.x << "," << viewPosition.y << std::endl;
	
	TWODAResource* startPosResource = gResManager->Get2DA("STARTPOS");
	if (startPosResource == NULL) {
		std::cout << "Failed!" << std::endl;
		gResManager->ReleaseResource(resource);
		return;
	}

	// TODO: Fix this
	/*if (Core::Get()->Game() == GAME_BALDURSGATE2) {
		// TODO: Needed for the initial script
		IE::point startPoint = {20, 20};
		fParty->AddActor(new Actor("IMOEN", startPoint, 0));
	}*/

	Core::Get()->LoadArea(startingArea.c_str(), "foo", NULL);
	Core::Get()->CurrentRoom()->SetAreaOffsetCenter(viewPosition);

	if (fParty != NULL) {
		for (int16 i = 0; i < fParty->CountActors(); i++) {
			char column[2];
			snprintf(column, sizeof(column), "%d", i + 1);			
			IE::point startPos;
			startPos.x = startPosResource->IntegerValueFor("START_XPOS", column);
			startPos.y = startPosResource->IntegerValueFor("START_YPOS", column);
			fParty->ActorAt(i)->SetPosition(startPos);
			fParty->ActorAt(i)->ClearDestination();
		}
	}
	
	gResManager->ReleaseResource(resource);	
	gResManager->ReleaseResource(startPosResource);
}


void
Game::ToggleDayNight()
{
	GameTimer::AdvanceTime(12, 0, 0);

	RoomBase* area = Core::Get()->CurrentRoom();
	if (area != NULL)
		area->ReloadArea();
	// TODO: Update Area
}


void
Game::SetTestMode(bool value)
{
	fTestMode = value;
}


bool
Game::TestMode() const
{
	return fTestMode;
}
