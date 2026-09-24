#include "ActionBar.h"

#include "2DAResource.h"
#include "Actor.h"
#include "AreaRoom.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "Button.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "ITMResource.h"
#include "ResManager.h"
#include "ScreenSupport.h"
#include "SPLResource.h"
#include "Window.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// The action bar's buttons are GemRB's ACT_* codes (ie_action.py), which
// are also the row numbers of GUIBTACT.2DA.
enum ActionCode {
	ACT_STEALTH = 0, ACT_THIEVING = 1, ACT_CAST = 2, ACT_QSPELL1 = 3,
	ACT_QSPELL2 = 4, ACT_QSPELL3 = 5, ACT_TURN = 6, ACT_TALK = 7,
	ACT_USE = 8, ACT_QSLOT1 = 9, ACT_QSLOT4 = 10, ACT_QSLOT2 = 11,
	ACT_QSLOT3 = 12, ACT_INNATE = 13, ACT_DEFEND = 14, ACT_ATTACK = 15,
	ACT_WEAPON1 = 16, ACT_WEAPON4 = 19, ACT_BARDSONG = 20, ACT_STOP = 21,
	ACT_SEARCH = 22, ACT_QSLOT5 = 31, ACT_NONE = 100
};
static const uint32 kActionButtons = 12;
static const uint32 kSlotQuickItemFirst = 18;	// QuickItem1-3 (18-20)

// Art of each action: frames of the first cycle (unpressed, pressed,
// selected, disabled) of GUIBTACT.BAM - GemRB's guibtact.2da (the games hardcode these numbers),
// indexed by ActionCode. The quick spell/item/weapon slots draw from
// GUIBTBUT.BAM instead, which is only used here for the empty slots.
static const uint16 kActionArt[][4] = {
	{ 30, 31, 32, 33 }, { 26, 27, 28, 29 }, { 12, 13, 52, 53 },	// stealth, thieving, cast
	{ 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 },			// quick spells
	{ 8, 9, 10, 11 }, { 4, 5, 6, 7 }, { 18, 19, 56, 57 },		// turn, talk, use item
	{ 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 },	// quick items
	{ 38, 39, 54, 55 }, { 0, 1, 2, 3 }, { 14, 15, 16, 17 },		// innate, defend, attack
	{ 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 },	// quick weapons
	{ 22, 23, 24, 25 }, { 58, 59, 60, 61 }, { 34, 35, 36, 37 },	// bard song, stop, search
};

// Frames of the guibtbut slots (quick spells 3-5, quick items 9-12, quick
// weapons 16-19): BG2's are a single empty-slot frame; BG1
// numbers them per slot.
static void
_GuibtbutCycles(uint32 action, bool bg1Layout, uint16 cycles[4])
{
	static const uint16 kBG1[][4] = {
		{ 8, 9, 32, 33 }, { 10, 11, 34, 35 }, { 12, 13, 36, 37 },	// quick spells
		{ 14, 15, 38, 39 }, { 16, 17, 40, 41 }, { 18, 19, 42, 43 },
		{ 20, 21, 44, 45 },						// quick items 1, 4, 2, 3
		{ 0, 1, 24, 25 }, { 2, 3, 26, 27 }, { 4, 5, 28, 29 },
		{ 6, 7, 30, 31 }						// quick weapons
	};
	int index = -1;
	if (action >= ACT_QSPELL1 && action <= ACT_QSPELL3)
		index = action - ACT_QSPELL1;
	else if (action == ACT_QSLOT1)
		index = 3;
	else if (action == ACT_QSLOT4)
		index = 4;
	else if (action == ACT_QSLOT2)
		index = 5;
	else if (action == ACT_QSLOT3)
		index = 6;
	else if (action >= ACT_WEAPON1 && action <= ACT_WEAPON4)
		index = 7 + (action - ACT_WEAPON1);
	for (int i = 0; i < 4; i++)
		cycles[i] = (bg1Layout && index >= 0) ? kBG1[index][i] : i;
}

// The classes' action rows from GemRB's qslots.2da: the first three
// buttons are always Talk and the first two weapons, then these nine.
struct class_actions {
	const char* name;
	uint8 actions[9];
};
static const class_actions kClassActions[] = {
	{ "MAGE", { 3, 4, 5, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER", { 18, 19, 14, 100, 8, 9, 11, 12, 13 } },
	{ "CLERIC", { 6, 3, 4, 2, 8, 9, 11, 12, 13 } },
	{ "THIEF", { 22, 1, 0, 100, 8, 9, 11, 12, 13 } },
	{ "BARD", { 20, 1, 3, 2, 8, 9, 11, 12, 13 } },
	{ "PALADIN", { 18, 14, 6, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_MAGE", { 3, 4, 5, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_CLERIC", { 6, 3, 4, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_THIEF", { 18, 22, 1, 0, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_MAGE_THIEF", { 22, 1, 0, 2, 8, 9, 11, 12, 13 } },
	{ "DRUID", { 3, 4, 5, 2, 8, 9, 11, 12, 13 } },
	{ "RANGER", { 18, 14, 0, 2, 8, 9, 11, 12, 13 } },
	{ "MAGE_THIEF", { 22, 1, 0, 2, 8, 9, 11, 12, 13 } },
	{ "CLERIC_MAGE", { 6, 3, 4, 2, 8, 9, 11, 12, 13 } },
	{ "CLERIC_THIEF", { 22, 1, 0, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_DRUID", { 3, 4, 5, 2, 8, 9, 11, 12, 13 } },
	{ "FIGHTER_MAGE_CLERIC", { 6, 3, 4, 2, 8, 9, 11, 12, 13 } },
	{ "CLERIC_RANGER", { 6, 3, 4, 2, 8, 9, 11, 12, 13 } },
	{ "SORCERER", { 3, 4, 5, 2, 8, 9, 11, 12, 13 } },
	{ "MONK", { 18, 14, 22, 0, 8, 9, 11, 12, 13 } },
};
// A creature whose class has no row gets this one.
static const uint8 kDefaultActions[9] = { 3, 4, 5, 2, 8, 9, 11, 12, 13 };


static void
_ActionRowFor(Actor* actor, uint8 row[kActionButtons])
{
	const uint8* actions = kDefaultActions;
	const std::string className = IDTable::ClassAt(actor->CRE()->Class());
	for (const class_actions& entry : kClassActions) {
		if (className == entry.name) {
			actions = entry.actions;
			break;
		}
	}
	row[0] = ACT_TALK;
	row[1] = ACT_WEAPON1;
	row[2] = ACT_WEAPON1 + 1;
	for (int i = 0; i < 9; i++)
		row[3 + i] = actions[i];
}


// CRE slot behind a quick item button (QuickItem1-3).
static uint32
_QuickItemSlot(uint32 action)
{
	switch (action) {
		case ACT_QSLOT1:
			return kSlotQuickItemFirst;
		case ACT_QSLOT2:
			return kSlotQuickItemFirst + 1;
		default:
			return kSlotQuickItemFirst + 2;
	}
}


// One entry of an action bar page: a memorized spell (one entry per spell,
// counting its copies) or a usable item and the inventory slot it sits in.
struct bar_entry {
	res_ref name;
	int32 slot;
	int count;
};

static std::vector<bar_entry>
_CastableSpells(Actor* actor, bool innate = false)
{
	std::vector<bar_entry> spells;
	for (const cre_memorized_spell& memorized : actor->CRE()->MemorizedSpells()) {
		if ((memorized.flags & 1) == 0)
			continue;
		const std::string name = memorized.spell.CString();
		// Innate abilities are the SPIN (and class SPCL) ones; the rest are
		// wizard/priest spells.
		const bool isInnate = name.compare(0, 4, "SPIN") == 0 || name.compare(0, 4, "SPCL") == 0;
		if (innate != isInnate || (!innate && name.compare(0, 4, "SPWI") != 0
				&& name.compare(0, 4, "SPPR") != 0))
			continue;
		bool found = false;
		for (bar_entry& known : spells) {
			if (known.name == memorized.spell) {
				known.count++;
				found = true;
			}
		}
		if (!found)
			spells.push_back({ memorized.spell, -1, 1 });
	}
	return spells;
}

// Items with a magical ability that still has something left to give:
// potions, scrolls, wands and the like, in the quick item, general and worn
// slots (weapons and ammunition are used by attacking).
static std::vector<bar_entry>
_UsableItems(Actor* actor)
{
	std::vector<bar_entry> items;
	for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
		if ((slot >= kSlotWeaponFirst && slot <= kSlotAmmoLast) || slot >= kSlotGeneralLast + 1)
			continue;
		IE::item item;
		if (!actor->CRE()->GetItemAtSlot(slot, item) || item.name.name[0] == '\0'
				|| item.quantity1 == 0)
			continue;
		ITMResource* itm = gResManager->GetITM(item.name);
		if (itm == NULL)
			continue;
		itm_ability ability;
		const bool usable = itm->GetAbility(0, ability) && ability.attackType == 3;
		gResManager->ReleaseResource(itm);
		if (usable)
			items.push_back({ item.name, (int32)slot, item.quantity1 });
	}
	return items;
}

// The spell/item page: the first button closes it (it wears the Cast or Use
// icon), then nine entries per page, with previous/next arrows in the last
// two buttons.
static const uint32 kEntriesPerPage = 9;
static const uint16 kArrowLeftArt[4] = { 44, 45, 44, 45 };
static const uint16 kArrowRightArt[4] = { 42, 43, 42, 43 };

static void
_ShowEntryPage(Window* window, const std::vector<bar_entry>& entries,
	uint32 header, bool spells, uint32 page, bool bg1Layout)
{
	for (uint32 i = 0; i < kActionButtons; i++) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(i));
		if (button == NULL)
			continue;
		button->SetIcon(NULL);
		button->SetIconCount(0);
		button->SetHighlighted(false);
		button->SetToggled(false);

		uint16 cycles[4];
		if (i == 0) {
			std::copy(kActionArt[header], kActionArt[header] + 4, cycles);
			button->SetArt(res_ref("GUIBTACT"), cycles);
			button->SetEnabled(true);
			button->SetHighlighted(true);
			continue;
		}
		const bool prev = i == kActionButtons - 2 && page > 0;
		const bool next = i == kActionButtons - 1 && (page + 1) * kEntriesPerPage < entries.size();
		if (prev || next) {
			button->SetArt(res_ref("GUIBTACT"), prev ? kArrowLeftArt : kArrowRightArt);
			button->SetEnabled(true);
			continue;
		}

		const size_t index = page * kEntriesPerPage + (i - 1);
		if (i > kEntriesPerPage || index >= entries.size()) {
			button->RestoreArt();
			button->SetEnabled(false);
			continue;
		}
		_GuibtbutCycles(spells ? ACT_QSPELL1 : ACT_QSLOT1, bg1Layout, cycles);
		cycles[3] = cycles[0];
		button->SetArt(res_ref("GUIBTBUT"), cycles);
		button->SetIcon(spells ? ScreenSupport::MakeSpellIcon(entries[index].name)
			: ScreenSupport::MakeItemIcon(entries[index].name), false);
		button->SetIconCount(entries[index].count);
		button->SetEnabled(true);
	}
}


void
ActionBar::Refresh()
{
	Window* window = GUI::Get()->GetWindow(GUI::WINDOW_CMDS);
	Actor* actor = fGame.ShownActor();
	if (window == NULL || actor == NULL || actor->CRE() == NULL)
		return;

	static int sBG1Layout = -1;
	if (sBG1Layout < 0) {
		// Only BG1's GUIBTBUT.BAM has the per-slot frames (up to 45).
		BAMResource* bam = gResManager->GetBAM(res_ref("GUIBTBUT"));
		sBG1Layout = 0;
		if (bam != NULL) {
			try {
				Bitmap* frame = bam->FrameForCycle(0, 45);
				if (frame != NULL) {
					frame->Release();
					sBG1Layout = 1;
				}
			} catch (...) {
			}
			gResManager->ReleaseResource(bam);
		}
	}

	uint8 row[kActionButtons];
	_ActionRowFor(actor, row);

	if (fPage != PAGE_ROW) {
		const bool spells = fPage != PAGE_ITEMS;
		const std::vector<bar_entry> entries = spells
			? _CastableSpells(actor, fPage == PAGE_INNATES) : _UsableItems(actor);
		uint32 header = ACT_USE;
		if (fPage == PAGE_SPELLS)
			header = ACT_CAST;
		else if (fPage == PAGE_INNATES)
			header = ACT_INNATE;
		if (entries.empty()) {
			fPage = PAGE_ROW; // nothing left to pick from
			fAssignQuickSpell = -1;
		}
		else
			_ShowEntryPage(window, entries, header, spells, fPageIndex, sBG1Layout == 1);
		if (fPage != PAGE_ROW)
			return;
	}

	for (uint32 i = 0; i < kActionButtons; i++) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(i));
		if (button == NULL)
			continue;

		const uint32 action = row[i];
		button->SetIcon(NULL);
		button->SetIconCount(0);
		button->SetHighlighted(false);

		const bool weapon = action >= ACT_WEAPON1 && action <= ACT_WEAPON4;
		if (action >= sizeof(kActionArt) / sizeof(kActionArt[0])) {
			button->RestoreArt();
			button->SetEnabled(false);
			continue;
		}

		if (weapon) {
			// The stone slot the CHU authored, with the item's icon on it.
			button->RestoreArt();
			const uint32 slot = kSlotWeaponFirst + (action - ACT_WEAPON1);
			IE::item item;
			const bool filled = actor->CRE()->GetItemAtSlot(slot, item);
			if (filled) {
				button->SetIcon(ScreenSupport::MakeItemIcon(item.name));
				button->SetIconCount(item.quantity1);
			}
			const bool inHand = filled && (int32)slot == actor->ActiveWeaponSlot();
			button->SetHighlighted(inHand);
			// Attack mode marks the weapon in hand with a second, inner
			// outline - drawn as the button being "toggled".
			button->SetToggled(inHand && fTargetMode == TARGET_ATTACK);
			button->SetEnabled(filled);
			continue;
		}

		const bool quickSpell = action >= ACT_QSPELL1 && action <= ACT_QSPELL3;
		const bool quickItem = action == ACT_QSLOT1 || action == ACT_QSLOT2
			|| action == ACT_QSLOT3;
		if (quickSpell || quickItem) {
			uint16 frames[4];
			_GuibtbutCycles(action, sBG1Layout == 1, frames);
			frames[3] = frames[0];
			button->SetArt(res_ref("GUIBTBUT"), frames);
			if (quickSpell) {
				const res_ref spell = actor->QuickSpell(action - ACT_QSPELL1);
				int left = 0;
				for (const bar_entry& entry : _CastableSpells(actor)) {
					if (entry.name == spell)
						left = entry.count;
				}
				if (spell.CString()[0] != '\0') {
					button->SetIcon(ScreenSupport::MakeSpellIcon(spell), false);
					button->SetIconCount(left);
				}
				// Always enabled: a right click assigns the slot, empty or not.
				button->SetEnabled(true);
			} else {
				const uint32 slot = _QuickItemSlot(action);
				bool usable = false;
				for (const bar_entry& entry : _UsableItems(actor)) {
					if ((uint32)entry.slot != slot)
						continue;
					usable = true;
					button->SetIcon(ScreenSupport::MakeItemIcon(entry.name), false);
					button->SetIconCount(entry.count);
				}
				button->SetEnabled(usable);
			}
			continue;
		}

		uint16 cycles[4];
		const bool guibtbut = (action >= ACT_QSPELL1 && action <= ACT_QSPELL3)
			|| (action >= ACT_QSLOT1 && action <= ACT_QSLOT3) || action == ACT_QSLOT4;
		if (guibtbut) {
			_GuibtbutCycles(action, sBG1Layout == 1, cycles);
			// An empty quick slot is just its plain frame - the disabled
			// one is a highlight, not a greyed-out slot.
			cycles[3] = cycles[0];
		} else
			std::copy(kActionArt[action], kActionArt[action] + 4, cycles);
		button->SetArt(res_ref(guibtbut ? "GUIBTBUT" : "GUIBTACT"), cycles);
		// Talk, Stop and Cast (when something is memorized) do something
		// so far; the rest show their icons greyed out.
		button->SetEnabled(action == ACT_STOP || action == ACT_TALK
			|| (action == ACT_CAST && !_CastableSpells(actor).empty())
			|| (action == ACT_USE && !_UsableItems(actor).empty())
			|| (action == ACT_INNATE && !_CastableSpells(actor, true).empty())
			|| action == ACT_DEFEND);
		button->SetHighlighted((action == ACT_TALK && fTargetMode == TARGET_TALK)
			|| (action == ACT_DEFEND && fTargetMode == TARGET_DEFEND));
	}
}


void
ActionBar::SetTargetMode(TargetMode mode)
{
	if (fTargetMode == mode)
		return;
	fTargetMode = mode;
	Refresh();
}


void
ActionBar::CastSpellAt(Actor* target)
{
	Actor* caster = fGame.ShownActor();
	if (caster != NULL && target != NULL) {
		if (fTargetMode == TARGET_USE_ITEM || fPendingItemSlot >= 0)
			caster->UseItem((uint32)fPendingItemSlot, target);
		else if (fPendingSpell.CString()[0] != '\0')
			caster->CastSpell(fPendingSpell, target);
	}
	fPendingSpell = res_ref();
	fPendingItemSlot = -1;
	SetTargetMode(TARGET_NONE);
}


void
ActionBar::_PickEntry(Actor* actor, const res_ref& name, int32 slot, bool spell)
{
	uint8 targetType = 0;
	if (spell) {
		fPendingSpell = name;
		fPendingItemSlot = -1;
		SPLResource* resource = gResManager->GetSPL(name.CString());
		if (resource != NULL) {
			targetType = resource->TargetType();
			gResManager->ReleaseResource(resource);
		}
	} else {
		fPendingSpell = res_ref();
		fPendingItemSlot = slot;
		ITMResource* resource = gResManager->GetITM(name);
		itm_ability ability;
		if (resource != NULL) {
			if (resource->GetAbility(0, ability))
				targetType = ability.targetType;
			gResManager->ReleaseResource(resource);
		}
	}
	const TargetMode mode = spell ? TARGET_CAST : TARGET_USE_ITEM;
	// Something that only affects its user needs no target.
	if (targetType == 0 || targetType == 5 || targetType == 7) {
		fTargetMode = mode;
		CastSpellAt(actor);
	} else {
		SetTargetMode(mode);
	}
}


void
ActionBar::ControlRightClicked(uint32 controlID)
{
	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL || controlID >= kActionButtons
			|| fPage != PAGE_ROW)
		return;

	uint8 row[kActionButtons];
	_ActionRowFor(actor, row);
	const uint32 action = row[controlID];
	if (action >= ACT_QSPELL1 && action <= ACT_QSPELL3 && !_CastableSpells(actor).empty()) {
		fTargetMode = TARGET_NONE;
		fAssignQuickSpell = action - ACT_QSPELL1;
		fPage = PAGE_SPELLS;
		fPageIndex = 0;
		Refresh();
	}
}


void
ActionBar::ControlInvoked(uint32 controlID)
{
	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL || controlID >= kActionButtons)
		return;

	if (fPage != PAGE_ROW) {
		const bool spells = fPage != PAGE_ITEMS;
		const std::vector<bar_entry> entries = spells
			? _CastableSpells(actor, fPage == PAGE_INNATES) : _UsableItems(actor);
		const uint32 prevButton = kActionButtons - 2, nextButton = kActionButtons - 1;
		if (controlID == 0) {
			fPage = PAGE_ROW;
			fAssignQuickSpell = -1;
		} else if (controlID == prevButton && fPageIndex > 0) {
			fPageIndex--;
		} else if (controlID == nextButton
				&& (fPageIndex + 1) * kEntriesPerPage < entries.size()) {
			fPageIndex++;
		} else if (controlID <= kEntriesPerPage
				&& fPageIndex * kEntriesPerPage + controlID - 1 < entries.size()) {
			const bar_entry entry = entries[fPageIndex * kEntriesPerPage + controlID - 1];
			fPage = PAGE_ROW;
			if (fAssignQuickSpell >= 0) {
				actor->SetQuickSpell((uint32)fAssignQuickSpell, entry.name);
				fAssignQuickSpell = -1;
			} else {
				_PickEntry(actor, entry.name, entry.slot, spells);
			}
		}
		Refresh();
		return;
	}

	uint8 row[kActionButtons];
	_ActionRowFor(actor, row);
	const uint32 action = row[controlID];
	if (action >= ACT_QSPELL1 && action <= ACT_QSPELL3) {
		const res_ref spell = actor->QuickSpell(action - ACT_QSPELL1);
		for (const bar_entry& entry : _CastableSpells(actor)) {
			if (entry.name == spell)
				_PickEntry(actor, spell, -1, true);
		}
		Refresh();
	} else if (action == ACT_QSLOT1 || action == ACT_QSLOT2 || action == ACT_QSLOT3) {
		const uint32 slot = _QuickItemSlot(action);
		for (const bar_entry& entry : _UsableItems(actor)) {
			if ((uint32)entry.slot == slot)
				_PickEntry(actor, entry.name, entry.slot, false);
		}
		Refresh();
	} else if (action == ACT_DEFEND) {
		SetTargetMode(fTargetMode == TARGET_DEFEND ? TARGET_NONE : TARGET_DEFEND);
	} else if (action == ACT_CAST || action == ACT_USE || action == ACT_INNATE) {
		const bool items = action == ACT_USE;
		if (!(items ? _UsableItems(actor) : _CastableSpells(actor, action == ACT_INNATE)).empty()) {
			fTargetMode = TARGET_NONE;
			if (items)
				fPage = PAGE_ITEMS;
			else if (action == ACT_INNATE)
				fPage = PAGE_INNATES;
			else
				fPage = PAGE_SPELLS;
			fPageIndex = 0;
			Refresh();
		}
	} else if (action >= ACT_WEAPON1 && action <= ACT_WEAPON4) {
		// Pressing the weapon already in hand asks whom to attack with it.
		const int32 slot = kSlotWeaponFirst + (action - ACT_WEAPON1);
		if (slot == actor->ActiveWeaponSlot()) {
			SetTargetMode(fTargetMode == TARGET_ATTACK ? TARGET_NONE : TARGET_ATTACK);
		} else {
			fTargetMode = TARGET_NONE;
			actor->SelectWeapon(action - ACT_WEAPON1);
			Refresh();
		}
	} else if (action == ACT_TALK) {
		SetTargetMode(fTargetMode == TARGET_TALK ? TARGET_NONE : TARGET_TALK);
	} else if (action == ACT_STOP) {
		fTargetMode = TARGET_NONE;
		actor->ClearActionList();
	}
}


ActionBar::ActionBar(Game& game)
	:
	fGame(game),
	fTargetMode(TARGET_NONE),
	fPage(PAGE_ROW),
	fPageIndex(0),
	fPendingItemSlot(-1),
	fAssignQuickSpell(-1)
{
}


void
ActionBar::ResetPage()
{
	fPage = PAGE_ROW;
	fAssignQuickSpell = -1;
}
