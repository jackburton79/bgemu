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
	Party();
	~Party();

	void AddActor(Actor* actor);
	void RemoveActor(Actor* actor);

	uint16 CountActors() const;
	Actor* ActorAt(uint16 index) const;

	bool HasActor(const Actor* actor) const;

	// StorePartyLocations()/RestorePartyLocations() - per IESDP these go
	// through the GAM file; here they're just kept in memory (this
	// engine's save format doesn't have a slot for them - see
	// resources/GamResource.h). Keyed by party order at store time, same
	// convention other party-wide actions already use. RestoreLocations()
	// clears the stored set afterwards (matches IESDP) and does nothing
	// if the party's membership changed since StoreLocations() (size
	// mismatch) rather than restoring to the wrong members.
	void StoreLocations();
	bool RestoreLocations();

	std::vector<Actor* > fActors;

private:
	std::vector<IE::point> fStoredLocations;
};

#endif /* PARTY_H_ */
