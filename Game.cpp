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
#include "GraphicsEngine.h"
#include "GUI.h"
#include "InputConsole.h"
#include "Party.h"
#include "OutputConsole.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Timer.h"

#include <stdio.h>

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
	fParty(NULL)
{
	fParty = new ::Party();
}


Game::~Game()
{
	delete fParty;
}


void
Game::Loop(bool executeScripts)
{
	std::cout << "Game::Loop()" << std::endl;
	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;

	//RoomContainer::Create();
	// TODO: Screenframe is 0 here, without the above commented call

	GFX::rect screenRect = GraphicsEngine::Get()->ScreenFrame();
	GUI* gui = GUI::Get();
	GFX::rect consoleRect(
			0,
			0,
			screenRect.w,
			screenRect.h - 21);

	std::cout << "Setting up output console... ";
	std::flush(std::cout);
	OutputConsole console(consoleRect, false);
	consoleRect.h = 20;
	consoleRect.y = screenRect.h - consoleRect.h;
	std::cout << "OK!" << std::endl;
	std::cout << "Setting up input console... ";
	std::flush(std::cout);
	InputConsole inputConsole(consoleRect);
	inputConsole.Initialize();
	std::cout << "OK!" << std::endl;

	// TODO: Move this elsewhere.
	// This should be filled by the player selection
	IE::point point = { 20, 20 };
	try {	
		if (fParty->CountActors() == 0) {
			if (Core::Get()->Game() == GAME_BALDURSGATE) {
				fParty->AddActor(new Actor("AJANTI", point, 0));
			} else {
				fParty->AddActor(new Actor("ANOMEN10", point, 0));
				fParty->AddActor(new Actor("IMOEN", point, 0));			
			}
		}
	} catch (...) {
		throw std::string("Error creating player!");
	}
	LoadStartingArea();

	std::cout << "Game: Started game loop." << std::endl;
	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		uint32 startTicks = Timer::Ticks();
		while (SDL_PollEvent(&event) != 0) {
			RoomBase* room = Core::Get()->CurrentRoom();
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					if (!Core::Get()->CutsceneMode())					
						gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					if (!Core::Get()->CutsceneMode())								
						gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					lastMouseX = event.motion.x;
					lastMouseY = event.motion.y;

					break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						console.Toggle();
						inputConsole.Toggle();
					} else if (console.IsActive()) {
						if (event.key.keysym.scancode < 0x80 && event.key.keysym.scancode > 0) {
							uint8 key = event.key.keysym.sym;
							if (event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
								key -= 32;
							inputConsole.HandleInput(key);
						}
					} else {
						switch (event.key.keysym.sym) {
							case SDLK_o:
								if (room != NULL)
									room->ToggleOverlays();
								break;
							case SDLK_d:
								if (console.HasOutputRedirected())
									console.DisableRedirect();
								else
									console.EnableRedirect();
								break;
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
								if (room != NULL)
									room->ToggleDayNight();
								break;
							case SDLK_q:
								quitting = true;
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

		// TODO: When MouseOver() doesn't draw anymore, reorder
		// these three calls. Draw() should be the last.
		gui->Draw();

		if (!Core::Get()->CutsceneMode())
			gui->MouseMoved(lastMouseX, lastMouseY);

		//console.Draw();
		inputConsole.Draw();
		Core::Get()->UpdateLogic(executeScripts);

		GraphicsEngine::Get()->Flip();
		
		Timer::WaitSync(startTicks, 35);
	}

	std::cout << "Game: Input loop stopped." << std::endl;
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
	IE::point point;
	point.x = resource->IntegerValueFor("START_XPOS", "VALUE");
	point.y = resource->IntegerValueFor("START_YPOS", "VALUE");

	std::cout << "Starting area: " << startingArea << std::endl;
	std::cout << "Starting position: " << point.x << "," << point.y << std::endl;

	Core::Get()->LoadArea(startingArea.c_str(), "foo", NULL);
	Core::Get()->CurrentRoom()->SetAreaOffsetCenter(point);
	if (fParty != NULL) {
		fParty->ActorAt(0)->SetPosition(point);
		fParty->ActorAt(0)->ClearDestination();
	}

	gResManager->ReleaseResource(resource);
}

