/*
 * FrontEnd.cpp - see FrontEnd.h
 */

#include "FrontEnd.h"

#include "Core.h"
#include "Game.h"
#include "GameConsole.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "MusPlaylist.h"
#include "PlaylistStream.h"
#include "SoundEngine.h"
#include "StartScreen.h"
#include "ScreenManager.h"
#include "IETypes.h"
#include "ResManager.h"

#include <SDL.h>

#include <iostream>
#include <vector>

// The intro movies as GemRB's GUIScripts play them (Start.py of each game).
static std::vector<const char*>
_IntroMovies()
{
	switch (Core::Get()->Game()) {
		case game::GAME_BALDURSGATE:
			return { "BG4LOGO", "TSRLOGO", "BILOGO", "INFELOGO", "INTRO" };
		case game::GAME_BALDURSGATE2:
			return { "BISLOGO", "BWDRAGON", "WOTC", "INTRO15F" };
		default:
			return {};
	}
}


/* static */
bool
FrontEnd::PlayIntroMovies()
{
	for (const char* name : _IntroMovies()) {
		if (!gResManager->ResourceExists(name, RES_MVE)) {
			std::cout << "FrontEnd: no intro movie " << name << std::endl;
			continue;
		}
		Core::Get()->PlayMovie(name);

		// The window was closed during the movie: the player put the event
		// back for the game loop.
		if (SDL_HasEvent(SDL_QUIT))
			return false;
	}
	return true;
}


// The menu's music: the theme playlist of the game's music/ directory, if it
// has one.
static void
_StartThemeMusic()
{
	SoundEngine* engine = SoundEngine::Get();
	MusPlaylist playlist;
	if (engine == NULL || !MusPlaylist::Load("Theme.mus", playlist))
		return;

	PlaylistStream* stream = new PlaylistStream(playlist);
	if (!stream->Valid()) {
		delete stream;
		return;
	}
	engine->PlayStream(stream, 1.0f, 1000);
}


/* static */
FrontEnd::MenuResult
FrontEnd::RunStartMenu(Game& game, GameConsole* console, const std::string& execFile)
{
	GUI* gui = GUI::Get();
	// Sets up what every screen needs (the cursors); the menu itself is an
	// auxiliary window like every other screen.
	gui->Load("START");

	StartScreen* start = game.Screens().Find<StartScreen>();
	GameScreen* load = game.Screens().Find("GUILOAD");
	start->Reset();
	start->Open();
	_StartThemeMusic();

	if (console != NULL && !execFile.empty()) {
		console->RunFile(execFile);
		gui->Draw();
		GraphicsEngine::Get()->Update();
		if (SoundEngine::Get() != NULL)
			SoundEngine::Get()->StopStream(0);
		start->Close();
		return MENU_QUIT;
	}

	MenuResult result = MENU_QUIT;
	bool running = true;
	bool loading = false;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_RIGHT)
						gui->RightMouseDown(event.button.x, event.button.y);
					else
						gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button != SDL_BUTTON_RIGHT)
						gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					gui->MouseMoved(event.motion.x, event.motion.y);
					break;
				case SDL_QUIT:
					running = false;
					break;
				default:
					break;
			}
		}

		// A game was loaded by the load screen: the area is up, the menu's
		// windows are gone.
		if (loading && Core::Get()->CurrentRoom() != NULL) {
			result = MENU_LOADED;
			break;
		}

		if (!loading) {
			switch (start->Selected()) {
				case StartScreen::CHOICE_NEW_GAME:
					result = MENU_NEW_GAME;
					running = false;
					break;
				case StartScreen::CHOICE_QUIT:
					result = MENU_QUIT;
					running = false;
					break;
				case StartScreen::CHOICE_LOAD_GAME:
					if (load != NULL) {
						loading = true;
						load->Open();
					} else
						start->Reset();
					break;
				default:
					break;
			}
		} else if (!load->IsOpen()) {
			// Cancelled: back to the menu.
			loading = false;
			start->Reset();
			start->Open();
		}

		if (!running)
			break;

		gui->Draw();
		GraphicsEngine::Get()->Update();
		SDL_Delay(16);
	}

	if (SoundEngine::Get() != NULL)
		SoundEngine::Get()->StopStream(result == MENU_LOADED ? 0 : 500);
	if (result != MENU_LOADED)
		start->Close();
	return result;
}
