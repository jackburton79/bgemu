/*
 * Party.cpp
 *
 *  Created on: 01/set/2013
 *      Author: stefano
 */

#include "Party.h"

#include "Core.h"

#include <algorithm>


Party::Party()
{
}


Party::~Party()
{
}


void
Party::AddActor(Actor* actor)
{
	if (actor != NULL) {
		fActors.push_back(actor);
		actor->SetInterruptable(false);
	}
}


void
Party::RemoveActor(Actor* actor)
{
	std::vector<Actor*>::iterator i =
		std::find(fActors.begin(), fActors.end(), actor);
	if (i != fActors.end()) {
		fActors.erase(i);
		//(i*)->SetInterruptable(true);
	}
}


uint16
Party::CountActors() const
{
	return fActors.size();
}


Actor*
Party::ActorAt(uint16 index) const
{
	if (index > CountActors() - 1)
		return NULL;
	return fActors[index];
}


bool
Party::HasActor(const Actor* actor) const
{
	std::vector<Actor*>::const_iterator i;
	for (i = fActors.begin(); i != fActors.end(); i++) {
		if ((*i)->Name() == actor->Name())
			return true;
	}
	return false;
}
