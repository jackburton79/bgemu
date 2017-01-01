/*
 * Game.cpp
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#include "Game.h"

#include "Actor.h"
#include "Core.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "InputConsole.h"
#include "OutputConsole.h"
#include "Party.h"
#include "Room.h"
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
	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;

	RoomContainer::Create();

	// TODO: Move this elsewhere.
	// This should be filled by the player selection
	IE::point point = { 20, 20 };
	if (fParty->CountActors() == 0) {
		if (Core::Get()->Game() == GAME_BALDURSGATE)
			fParty->AddActor(new Actor("AJANTI", point, 0));
		else
			fParty->AddActor(new Actor("AESOLD", point, 0));
	}

	if (!RoomContainer::Get()->LoadWorldMap()) {
		std::cerr << "LoadWorldMap failed" << std::endl;
		return;
	}

	GFX::rect screenRect = GraphicsEngine::Get()->ScreenFrame();
	GUI* gui = GUI::Get();
	GFX::rect consoleRect(
			0,
			0,
			screenRect.w,
			screenRect.h - 21);

	OutputConsole* console = new OutputConsole(consoleRect);
	consoleRect.h = 20;
	consoleRect.y = screenRect.h - consoleRect.h;
	InputConsole* inputConsole = new InputConsole(consoleRect);
	inputConsole->Initialize();

	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			RoomContainer* room = RoomContainer::Get();
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					lastMouseX = event.motion.x;
					lastMouseY = event.motion.y;

					break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						console->Toggle();
						inputConsole->Toggle();
					} else if (console->IsActive()) {
						/*if (event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0) {
							uint8 key = event.key.keysym.sym;
							if (event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
								key -= 32;
							inputConsole->HandleInput(key);
						}*/
					} else {
						switch (event.key.keysym.sym) {
							case SDLK_o:
								if (room != NULL)
									room->ToggleOverlays();
								break;
							case SDLK_d:
								if (console->HasOutputRedirected())
									console->DisableRedirect();
								else
									console->EnableRedirect();
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
								if (room != NULL)
									room->LoadWorldMap();
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

		// TODO: needs to be called at every loop, not only when the mouse
		// is moved
		gui->MouseMoved(lastMouseX, lastMouseY);

		console->Draw();
		inputConsole->Draw();
		Core::Get()->UpdateLogic(executeScripts);

		GraphicsEngine::Get()->Flip();
		if (inputConsole->IsActive())
			Timer::Wait(25);
		else
			Timer::Wait(50);
	}

	delete console;
	delete inputConsole;
}


Party*
Game::Party()
{
	return fParty;
}
