/*
 * FrontEnd.h
 *
 * What comes before a game: the intro movies (the logos and the introduction),
 * then - to come - the start menu and the character creation.
 */

#pragma once

class FrontEnd {
public:
	// The logo movies and the introduction of the game being run, in the order
	// the original plays them; Escape/Space/Return/a click skips one. Movies the
	// installation doesn't have are left out. Returns false if the window was
	// closed meanwhile.
	static bool PlayIntroMovies();
};
