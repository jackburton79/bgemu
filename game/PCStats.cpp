#include "PCStats.h"

#include <algorithm>


// Counts one use of `resref` among the favourites: its own slot if it has one,
// else the slot of the least used, which it takes over.
static void
_Register(res_ref* refs, uint16* counts, const res_ref& resref)
{
	if (resref.CString()[0] == '\0')
		return;
	int slot = -1;
	for (int i = 0; i < PCStats::kFavourites; i++) {
		if (refs[i] == resref) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		slot = (int)(std::min_element(counts, counts + PCStats::kFavourites) - counts);
		refs[slot] = resref;
		counts[slot] = 0;
	}
	if (counts[slot] < 0xffff)
		counts[slot]++;
}


static res_ref
_Favourite(const res_ref* refs, const uint16* counts)
{
	const int best = (int)(std::max_element(counts, counts + PCStats::kFavourites) - counts);
	return counts[best] > 0 ? refs[best] : res_ref();
}


void
PCStats::NotifyKill(uint32 xp, uint32 nameStrRef)
{
	chapterXP += xp;
	chapterKills++;
	totalXP += xp;
	totalKills++;
	if (xp > bestKilledXP || bestKilledName == kNoName) {
		bestKilledXP = xp;
		bestKilledName = nameStrRef;
	}
}


void
PCStats::StartChapter()
{
	chapterXP = 0;
	chapterKills = 0;
}


void
PCStats::RegisterSpell(const res_ref& spell)
{
	_Register(spells, spellCounts, spell);
}


void
PCStats::RegisterWeapon(const res_ref& weapon)
{
	_Register(weapons, weaponCounts, weapon);
}


res_ref
PCStats::FavouriteSpell() const
{
	return _Favourite(spells, spellCounts);
}


res_ref
PCStats::FavouriteWeapon() const
{
	return _Favourite(weapons, weaponCounts);
}
