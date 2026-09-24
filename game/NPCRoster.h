#pragma once

#include "IETypes.h"

#include <vector>

class Actor;
class GamResource;
class Party;


// The global NPCs: the creatures outside the party that the game itself
// keeps track of (a new game's companions and story characters, or whoever
// left the party) - unlike an area's other actors they aren't part of any
// area's own state: each one just remembers the area and point it is at
// (Actor::AreaName(), Position()) and shows up when that area is loaded.
// The roster holds one reference to each.
class NPCRoster {
public:
	~NPCRoster();

	uint16 Count() const;
	Actor* At(uint16 index) const;
	bool Contains(const Actor* actor) const;
	// The NPC a script's object name refers to (its CRE name or death
	// variable), NULL if none.
	Actor* Find(const char* name) const;

	// Takes over the caller's reference.
	void Add(Actor* actor);
	// Drops the roster's reference.
	void Remove(Actor* actor);
	void Clear();

	// An actor of the current area becomes a global NPC (MAKEGLOBAL): the
	// area stops placing it, the roster keeps it from now on.
	void Adopt(Actor* actor);
	// Moves an NPC to a point of an area, which needn't be loaded: out of
	// the room it is in if that isn't the destination, into the current
	// room if it is.
	void Move(Actor* npc, const res_ref& area, const IE::point& position,
		int orientation);

	// Fills the roster from a GAM's out-of-party table (a new game's
	// BALDUR.GAM or a save), skipping anyone already in `party`.
	void Load(GamResource* gam, const Party* party);
	// A new game starts from BALDUR.GAM: its out-of-party NPC table says
	// where every companion and story character begins.
	void LoadStarting(const Party* party);

private:
	std::vector<Actor*> fNPCs;
};
