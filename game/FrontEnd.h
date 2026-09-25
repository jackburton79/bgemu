/*
 * FrontEnd.h
 *
 * What comes before a game, as a sequence of steps run in order until the
 * player has a game to play: the intro movies (the logos and the
 * introduction), then the start menu, from which the player starts a new game,
 * loads a saved one or leaves. (The character creation will be the step after
 * "New Game".) Each step is a method with its own loop; they share the frame
 * and event handling below, and the game's own loop uses the same mouse
 * dispatch (InputEvents.h).
 */

#pragma once

#include <string>

class Game;
class GameConsole;
class StartScreen;

class FrontEnd {
public:
	enum Result {
		NEW_GAME,	// start a new game
		LOADED,		// a saved game was loaded from the menu (the area is in place)
		QUIT		// leave (also: the window was closed)
	};

	// `playIntro` starts with the movies. With a console script (`execFile`, see
	// Game::SetExecFile()) the script runs while the menu is up and the front end
	// then ends as QUIT: the way to test the menu.
	FrontEnd(Game& game, GameConsole* console, const std::string& execFile,
		bool playIntro);

	Result Run();

private:
	enum Step { STEP_INTRO, STEP_MENU, STEP_DONE };

	// Steps: each returns what to do next (or sets fResult and ends the run).
	Step _PlayIntroMovies();
	Step _RunStartMenu();

	// One turn of a step's loop: hands the pending events to the GUI (false if
	// the window was closed) and shows the frame.
	bool _PollEvents();
	void _ShowFrame();

	Game& fGame;
	GameConsole* fConsole;
	std::string fExecFile;
	bool fPlayIntro;
	Result fResult;
};
