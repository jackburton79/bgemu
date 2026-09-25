#pragma once

#include "IETypes.h"


// What the Information window of the character sheet tells about a party
// member: the kills, the favourite spell and weapon, when the character
// joined. The GAM keeps it in the member's "character stats" block (0xe4 of the
// party struct, 116 bytes) with the same four-slot favourites.
struct PCStats {
	static const uint32 kNoName = 0xffffffff;
	static const int kFavourites = 4;

	// The most powerful creature killed: the strref of its name and the XP it
	// was worth.
	uint32 bestKilledName = kNoName;
	uint32 bestKilledXP = 0;
	// Game seconds (GameTimer::GameTime()) when the character joined.
	uint32 joinTime = 0;
	// XP and number of the kills in this chapter and in the whole game.
	uint32 chapterXP = 0;
	uint32 chapterKills = 0;
	uint32 totalXP = 0;
	uint32 totalKills = 0;
	// The spells cast and the weapons used the most, with how many times.
	res_ref spells[kFavourites];
	uint16 spellCounts[kFavourites] = {};
	res_ref weapons[kFavourites];
	uint16 weaponCounts[kFavourites] = {};

	// A kill worth `xp` of a creature named by `nameStrRef`.
	void NotifyKill(uint32 xp, uint32 nameStrRef);
	// The chapter's kills start again from nothing.
	void StartChapter();
	void RegisterSpell(const res_ref& spell);
	void RegisterWeapon(const res_ref& weapon);
	// The one cast / used the most (an empty resref if none).
	res_ref FavouriteSpell() const;
	res_ref FavouriteWeapon() const;
};
