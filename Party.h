/*
 * Party.h
 *
 *  Created on: 01/set/2013
 *      Author: Stefano Ceccherini
 */

#ifndef PARTY_H_
#define PARTY_H_

#include <vector>

#include "Actor.h"

class Party {
public:
	static Party* Get();

	void AddActor(Actor* actor);
	void RemoveActor(Actor* actor);

	uint16 CountActors() const;
	Actor* ActorAt(uint16 index) const;

	bool HasActor(Actor* actor) const;

private:
	Party();
	~Party();

	std::vector<Actor*> fActors;
};

#endif /* PARTY_H_ */
