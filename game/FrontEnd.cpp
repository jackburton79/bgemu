/*
 * FrontEnd.cpp - see FrontEnd.h
 */

#include "FrontEnd.h"

#include "Core.h"
#include "CharGenScreen.h"
#include "Game.h"
#include "GameConsole.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "InputEvents.h"
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


FrontEnd::FrontEnd(Game& game, GameConsole* console, const std::string& execFile,
	bool playIntro)
	:
	fGame(game),
	fConsole(console),
	fExecFile(execFile),
	fPlayIntro(playIntro),
	fResult(QUIT)
{
}


FrontEnd::Result
FrontEnd::Run()
{
	Step step = fPlayIntro ? STEP_INTRO : STEP_MENU;
	while (step != STEP_DONE) {
		switch (step) {
			case STEP_INTRO:
				step = _PlayIntroMovies();
				break;
			case STEP_MENU:
				step = _RunStartMenu();
				break;
			case STEP_CHARGEN:
				step = _RunCharacterCreation();
				break;
			default:
				step = STEP_DONE;
				break;
		}
	}
	return fResult;
}


// The logo movies and the introduction of the game being run, in the order the
// original plays them; a movie the installation doesn't have is left out.
// Closing the window during one ends the run.
FrontEnd::Step
FrontEnd::_PlayIntroMovies()
{
	for (const char* name : _IntroMovies()) {
		if (!gResManager->ResourceExists(name, RES_MVE)) {
			std::cout << "FrontEnd: no intro movie " << name << std::endl;
			continue;
		}
		Core::Get()->PlayMovie(name);

		// MoviePlayer put the event back for whoever polls next.
		if (SDL_HasEvent(SDL_QUIT)) {
			fResult = QUIT;
			return STEP_DONE;
		}
	}
	return STEP_MENU;
}


bool
FrontEnd::_PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		if (DispatchMouseEvent(GUI::Get(), event))
			continue;
		if (event.type == SDL_QUIT)
			return false;
	}
	return true;
}


void
FrontEnd::_ShowFrame()
{
	GUI::Get()->Draw();
	GraphicsEngine::Get()->Update();
	SDL_Delay(16);
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


// The start menu (StartScreen) with the game's theme music, until something is
// chosen. Load Game opens the load screen from here: a game loaded by it ends
// the step as LOADED, cancelling it comes back to the menu.
FrontEnd::Step
FrontEnd::_RunStartMenu()
{
	GUI* gui = GUI::Get();
	// Sets up what every screen needs (the cursors); the menu itself is an
	// auxiliary window like every other screen.
	gui->Load("START");

	// No area is there to show the GUI, and with it the cursor.
	gui->SetCursorVisible(true);

	// The pointer is already somewhere: draw the cursor there, not in the corner
	// until the first movement.
	int pointerX = 0, pointerY = 0;
	SDL_GetMouseState(&pointerX, &pointerY);
	gui->MouseMoved(pointerX, pointerY);

	StartScreen* start = fGame.Screens().Find<StartScreen>();
	GameScreen* load = fGame.Screens().Find("GUILOAD");
	start->Reset();
	start->Open();
	_StartThemeMusic();

	fResult = QUIT;
	if (fConsole != NULL && !fExecFile.empty()) {
		fConsole->RunFile(fExecFile);
		gui->Draw();
		GraphicsEngine::Get()->Update();
		if (SoundEngine::Get() != NULL)
			SoundEngine::Get()->StopStream(0);
		start->Close();
		gui->SetCursorVisible(false);
		return STEP_DONE;
	}

	bool running = true;
	bool loading = false;
	while (running) {
		if (!_PollEvents())
			break;

		// A game was loaded by the load screen: the area is up, the menu's
		// windows are gone.
		if (loading && Core::Get()->CurrentRoom() != NULL) {
			fResult = LOADED;
			break;
		}

		if (!loading) {
			switch (start->Selected()) {
				case StartScreen::CHOICE_NEW_GAME:
					fResult = NEW_GAME;
					running = false;
					break;
				case StartScreen::CHOICE_QUIT:
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

		if (running)
			_ShowFrame();
	}

	if (SoundEngine::Get() != NULL)
		SoundEngine::Get()->StopStream(fResult == LOADED ? 0 : 500);
	if (fResult != LOADED)
		start->Close();
	// The character creation of a new game, where the game has one.
	if (fResult == NEW_GAME && Core::Get()->Game() == game::GAME_BALDURSGATE)
		return STEP_CHARGEN;
	GUI::Get()->SetCursorVisible(false);
	return STEP_DONE;
}


// The character creation (CharGenScreen) until it is accepted (NEW_GAME, with the
// character in the game's builder) or cancelled (back to the menu).
FrontEnd::Step
FrontEnd::_RunCharacterCreation()
{
	GUI* gui = GUI::Get();
	gui->SetCursorVisible(true);

	CharGenScreen* creation = fGame.Screens().Find<CharGenScreen>();
	creation->Begin();
	while (creation->Result() == CharGenScreen::OUTCOME_NONE) {
		if (!_PollEvents()) {
			fResult = QUIT;
			gui->SetCursorVisible(false);
			return STEP_DONE;
		}
		_ShowFrame();
	}

	const bool done = creation->Result() == CharGenScreen::OUTCOME_DONE;
	creation->Close();
	if (!done)
		return STEP_MENU;
	fResult = NEW_GAME;
	gui->SetCursorVisible(false);
	return STEP_DONE;
}
