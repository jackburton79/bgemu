#pragma once

#include "IETypes.h"

#include <string>

class Actor;
class CREResource;
class Game;
struct gam_party_member;


// Saving and loading the game: where the saves live, and turning the live
// session (party, out-of-party NPCs, variables, time, journal, the areas
// checkpointed so far) into a GAM file plus the area checkpoints next to it,
// and back.
class SavedGame {
public:
	SavedGame(Game& game);

	// Directory holding everything this engine writes for saving: one
	// "savegame_slot<N>.gam" (+ ".arecache" directory) per slot and the
	// session's own area checkpoints ("current/arecache", see
	// AreaRoom::AreaCheckpointDir()). Set once at startup (bgemu.cpp), per
	// game installation. Every save (the Save/Load screens' and
	// SAVEGAME(190)'s) is SlotPath(index), under Directory().
	void SetDirectory(const std::string& path);
	const std::string& Directory() const;
	std::string SlotPath(uint32 index) const;

	bool Save(const char* name);
	bool Load(const char* name);

	// A character as a save (or BALDUR.GAM) describes it: the CRE's own
	// files, with the saved CRE state (if any - taken over and released
	// here) on top.
	static Actor* RestoreActor(const gam_party_member& member, CREResource* savedCre);
	static gam_party_member MemberFor(Actor* actor, const res_ref& areaName);

private:
	Game& fGame;
	std::string fDirectory;
};
