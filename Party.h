/*
 * Party.h
 *
 *  Created on: 01/set/2013
 *      Author: Stefano Ceccherini
 */

#ifndef PARTY_H_
#define PARTY_H_

#include <list>

#include "Actor.h"

class Party {
public:
	Party();
	~Party();

	static Party* Get();

	void AddActor(Actor* actor);
	void RemoveActor(Actor* actor);
	bool HasActor(Actor* actor) const;
private:

	std::list<Actor*> fActors;
};

#endif /* PARTY_H_ */
