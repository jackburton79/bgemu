#pragma once

#include "IETypes.h"

#include <string>
#include <vector>

class Actor;
class Bitmap;
class CREResource;
class ITMResource;
class Window;


// Helpers several screens (and the parts of Game that aren't screens yet)
// share.
namespace ScreenSupport {

// The encumbrance ("bag") icon and its two current/max weight labels, on
// the inventory, loot and store windows. Unlike every other label on them,
// these two aren't CHU-authored at all - real BG2 creates them at runtime
// anchored to the bag icon's own rect (see GemRB's GUIINV.py "encumbrance"
// section) - same approach in EnsureWeightLabels().
constexpr uint32 kWeightIconID = 67;
constexpr uint32 kWeightCurrentLabelID = 268435523;
constexpr uint32 kWeightMaxLabelID = 268435524;

// Builds the inventory icon (cycle 0 / frame 0 of the ITM's inventory-icon
// BAM, same convention Button uses for its own CHU bitmaps) for an item
// resref. Returns a new reference the caller owns, or NULL.
Bitmap* MakeItemIcon(const res_ref& itemName);

// Human-readable name for an item: its identified name, else its
// unidentified name, else the bare resref. The second form resolves the ITM
// itself, for callers that only have the resref.
std::string ItemDisplayName(ITMResource* itm, const res_ref& itemName);
std::string ItemDisplayName(const res_ref& itemName);

// One item as listed in the loot and store windows, plus where it lives in
// its owner: an index into a Container's item list, or a CRE item slot.
struct LootEntry { IE::item item; int32 slot; };

// What `looter` carries in its general inventory slots (the ones the loot
// and store windows list on the right - equipped gear isn't offered).
void CollectOwnEntries(Actor* looter, std::vector<LootEntry>& entries);

// Sum of every item the CRE carries, and its STR-based carry capacity.
uint32 CarriedWeight(CREResource* cre);
uint32 MaxEncumbrance(CREResource* cre);

// A label that isn't CHU-authored, created at runtime in `window`.
void AddLabel(Window* window, uint32 id, sint16 x, sint16 y, uint16 width,
	uint16 height, uint16 flags);

// Creates the two weight labels once, anchored to the bag icon.
void EnsureWeightLabels(Window* window, uint32 iconID = kWeightIconID);

// The spellbook-icon frame for a spell resref (SPL 0x3a -> BAM cycle 0
// frame 0). Caller owns the returned reference, or NULL.
Bitmap* MakeSpellIcon(const res_ref& spellName);

}
