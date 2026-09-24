#pragma once

#include "IETypes.h"

class Actor;
class Game;


// The HUD action bar (GUIW's WINDOW_CMDS, 12 buttons) for the shown party
// member: the class's row of actions with the four weapon quickslots
// working, or - after Cast/Innate/Use - a page listing what the character
// can cast or use. It is part of the HUD's own resource, not a screen.
class ActionBar {
public:
	ActionBar(Game& game);

	// Fills the bar for the shown party member. Call whenever the shown
	// character, their weapons or inventory, or the HUD itself change.
	void Refresh();
	// Back to the class row, and no quick spell being assigned - what
	// choosing another character does.
	void ResetPage();

	// What the next click in the area does, chosen from the action bar:
	// talk to / attack the creature clicked instead of the usual
	// friend-or-foe guess. One-shot: any click in the area ends it.
	enum TargetMode { TARGET_NONE, TARGET_TALK, TARGET_ATTACK, TARGET_CAST,
		TARGET_USE_ITEM, TARGET_DEFEND };
	TargetMode CurrentTargetMode() const { return fTargetMode; }
	void SetTargetMode(TargetMode mode);
	// Ends TARGET_CAST/TARGET_USE_ITEM: the shown character casts the spell
	// (or uses the item) picked from the action bar at `target`.
	void CastSpellAt(Actor* target);

	// GUI routes clicks on the bar here.
	void ControlInvoked(uint32 controlID);
	// A right click on a quick spell button offers the spell page to assign
	// one to it.
	void ControlRightClicked(uint32 controlID);

private:
	// What the bar shows: the class row, or a page listing the shown
	// character's memorized spells / innate abilities / usable items (paged
	// by fPageIndex).
	enum ActionPage { PAGE_ROW, PAGE_SPELLS, PAGE_INNATES, PAGE_ITEMS };

	// Casts `spell` / uses the item in `slot` for `actor`, asking for a
	// target first unless it only affects its user.
	void _PickEntry(Actor* actor, const res_ref& name, int32 slot, bool spell);

	Game& fGame;
	TargetMode fTargetMode;
	ActionPage fPage;
	uint32 fPageIndex;
	// The spell or item slot picked on the bar, waiting for its target.
	int32 fPendingItemSlot;
	res_ref fPendingSpell;
	// Quick spell slot (0-2) the spell page is choosing a spell for, or -1.
	int32 fAssignQuickSpell;
};
