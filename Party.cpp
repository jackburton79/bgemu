/*
 * Party.cpp
 *
 *  Created on: 01/set/2013
 *      Author: stefano
 */

#include "Party.h"

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
	Reference<Actor> ref(actor);
	std::vector<Reference<Actor> >::iterator i =
		std::find(fActors.begin(), fActors.end(), ref);
	if (i != fActors.end()) {
		fActors.erase(i);
		i->Target()->SetInterruptable(true);
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
	return fActors[index].Target();
}


bool
Party::HasActor(const Actor* actor) const
{
	Reference<Actor> ref(const_cast<Actor*>(actor));
	std::vector<Reference<Actor> >::const_iterator i
		= std::find(fActors.begin(), fActors.end(), ref);
	return i != fActors.end();
}
