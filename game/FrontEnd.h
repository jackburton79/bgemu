/*
 * FrontEnd.h
 *
 * What comes before a game: the intro movies (the logos and the introduction),
 * then the start menu (and, to come, the character creation).
 */

#pragma once

#include <string>

class Game;
class GameConsole;

class FrontEnd {
public:
	enum MenuResult {
		MENU_NEW_GAME,	// start a new game
		MENU_LOADED,	// a saved game was loaded from the menu
		MENU_QUIT		// leave (also: the window was closed)
	};

	// The logo movies and the introduction of the game being run, in the order
	// the original plays them; Escape/Space/Return/a click skips one. Movies the
	// installation doesn't have are left out. Returns false if the window was
	// closed meanwhile.
	static bool PlayIntroMovies();

	// The start menu, with the game's theme music, until something is chosen.
	// Load Game opens the load screen from here and MENU_LOADED means a game
	// was loaded (the area is in place); New Game leaves the game to the caller.
	// With a console script (`execFile`, see Game::SetExecFile()) the script runs
	// while the menu is up, then the menu ends as a MENU_QUIT: the way to test it.
	static MenuResult RunStartMenu(Game& game, GameConsole* console,
		const std::string& execFile);
};
