/*
 * FrontEnd.cpp - see FrontEnd.h
 */

#include "FrontEnd.h"

#include "Core.h"
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
			return { "BISLOGO", "BWDRAGON", "WOTC" };
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
