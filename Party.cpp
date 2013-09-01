/*
 * Party.cpp
 *
 *  Created on: 01/set/2013
 *      Author: stefano
 */

#include "Party.h"

#include <algorithm>

static Party* sParty = NULL;


Party::Party()
{
}


Party::~Party()
{
}


/* static */
Party*
Party::Get()
{
	if (sParty == NULL)
		sParty = new Party();
	return sParty;
}


void
Party::AddActor(Actor* actor)
{
	fActors.push_back(actor);
}


void
Party::RemoveActor(Actor* actor)
{
	fActors.remove(actor);
}


bool
Party::HasActor(Actor* actor) const
{
	std::list<Actor*>::const_iterator i
		= std::find(fActors.begin(), fActors.end(), actor);
	return i != fActors.end();
}
