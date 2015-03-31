/*
 * Game.cpp
 *
 *  Created on: 29/mar/2015
 *      Author: stefano
 */

#include "Game.h"

#include <Actor.h>
#include <Core.h>
#include <Party.h>

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
	// TODO: Move this elsewhere.
	// This should be filled by the player selection
	fParty = new ::Party();
	IE::point point = { 20, 20 };
	if (fParty->CountActors() == 0) {
		if (Core::Get()->Game() == GAME_BALDURSGATE)
			fParty->AddActor(new Actor("AJANTI", point, 0));
		else
			fParty->AddActor(new Actor("AESOLD", point, 0));
	}
}


Game::~Game()
{
	delete fParty;
}


Party*
Game::Party()
{
	return fParty;
}
