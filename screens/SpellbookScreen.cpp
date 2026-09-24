#include "SpellbookScreen.h"

#include "Actor.h"
#include "Button.h"
#include "CreResource.h"
#include "Game.h"
#include "GUI.h"
#include "Label.h"
#include "ResManager.h"
#include "ScreenSupport.h"
#include "SPLResource.h"
#include "TextArea.h"
#include "Window.h"

#include <string>
#include <vector>

// GUIMG/GUIPR window-2 control ids (identical layout, from the CHU dump):
// left page = the memorized-spell grid (3-col, ids 3-14), right page =
// the known-spell grid (4-col, ids 27-38).
static const uint32 kSpellMemoControls[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
static const uint32 kSpellKnownControls[] = { 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38 };
static const uint32 kSpellNameLabelID = 268435509;


SpellbookScreen::SpellbookScreen(Game& game, bool divine)
	:
	PanelScreen(game, divine ? "GUIPR" : "GUIMG"),
	fDivine(divine),
	fLevel(1)
{
}


/* virtual */
uint32
SpellbookScreen::CommandBarButton() const
{
	return fDivine ? 6 : 5;
}


// Always opens on level 1.
/* virtual */
void
SpellbookScreen::OnOpen()
{
	fLevel = 1;
}


// The spell-info popup goes with the book.
/* virtual */
void
SpellbookScreen::OnClose()
{
	GUI::Get()->HideAuxWindow(CHUName(), kInfoWindow);
}




// Fills the known/memorized grids with the shown character's spells of the
// current level (arcane or divine, by which of the two books this is),
// remembering which spell each grid button shows so PanelControlInvoked()
// can act on a click.
/* virtual */
void
SpellbookScreen::RefreshContent()
{
	fKnown.clear();
	fMemo.clear();

	const uint16 spellType = fDivine ? 0 : 1; // 0 priest, 1 wizard
	Window* window = GetWindow(kContentWindow);
	Actor* actor = fGame.ShownActor();
	if (window == NULL || actor == NULL || actor->CRE() == NULL)
		return;
	CREResource* cre = actor->CRE();

	const uint16 level = fLevel;

	Label* nameLabel = dynamic_cast<Label*>(window->GetControlByID(kSpellNameLabelID));
	if (nameLabel != NULL)
		nameLabel->SetText(actor->LongName() + " - level " + std::to_string(level));

	// Known spells of the current level/type.
	std::vector<cre_known_spell> known = cre->KnownSpells();
	size_t k = 0;
	for (uint32 controlID : kSpellKnownControls) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
		if (button == NULL)
			continue;
		while (k < known.size() && (known[k].type != spellType || known[k].level != level))
			k++;
		if (k < known.size()) {
			button->SetIcon(ScreenSupport::MakeSpellIcon(known[k].spell), true);
			fKnown[controlID] = known[k].spell;
			k++;
		} else {
			button->SetIcon(NULL);
		}
	}

	// Memorized spells of the current level/type - the memo-info rows
	// point at each level/type's slice of the memorized table.
	std::vector<cre_memorized_spell> memo = cre->MemorizedSpells();
	std::vector<cre_spell_memorization_info> info = cre->SpellMemorizationInfo();
	std::vector<cre_memorized_spell> levelMemo;
	for (const cre_spell_memorization_info& row : info) {
		if (row.level != level || row.type != spellType)
			continue;
		for (uint32 i = 0; i < row.memorizedCount; i++) {
			uint32 idx = row.firstMemorizedIndex + i;
			if (idx < memo.size())
				levelMemo.push_back(memo[idx]);
		}
	}
	size_t m = 0;
	for (uint32 controlID : kSpellMemoControls) {
		Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
		if (button == NULL)
			continue;
		if (m < levelMemo.size()) {
			button->SetIcon(ScreenSupport::MakeSpellIcon(levelMemo[m].spell), true);
			if (levelMemo[m].flags & 1)
				fMemo[controlID] = levelMemo[m].spell;
			m++;
		} else {
			button->SetIcon(NULL);
		}
	}
}


/* virtual */
void
SpellbookScreen::PanelControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kInfoWindow) {
		// The spell-info popup: any button closes it.
		GUI::Get()->HideAuxWindow(CHUName(), kInfoWindow);
		return;
	}
	if (windowID != kContentWindow)
		return;

	// Page arrows: control 1 = previous spell level, control 2 = next.
	if (controlID == 1 || controlID == 2) {
		if (controlID == 1 && fLevel > 1)
			fLevel--;
		else if (controlID == 2 && fLevel < 9)
			fLevel++;
		RefreshContent();
		return;
	}

	Actor* actor = fGame.ShownActor();
	if (actor == NULL || actor->CRE() == NULL)
		return;

	// Click a known spell -> memorize it into a free slot; click a
	// memorized spell -> release it (un-memorize).
	auto known = fKnown.find(controlID);
	if (known != fKnown.end()) {
		actor->CRE()->MemorizeSpell(known->second);
		RefreshContent();
		return;
	}
	auto memo = fMemo.find(controlID);
	if (memo != fMemo.end()) {
		actor->CRE()->ConsumeMemorizedSpell(memo->second);
		RefreshContent();
	}
}


/* virtual */
bool
SpellbookScreen::ControlHovered(uint16 windowID, uint32 controlID, bool inside)
{
	if (!inside) {
		GUI::Get()->SetHoverTooltip("");
		return true;
	}
	if (windowID != kContentWindow)
		return false;
	res_ref spell;
	auto known = fKnown.find(controlID);
	auto memo = fMemo.find(controlID);
	if (known != fKnown.end())
		spell = known->second;
	else if (memo != fMemo.end())
		spell = memo->second;
	else {
		GUI::Get()->SetHoverTooltip("");
		return true;
	}

	std::string name = spell.CString();
	SPLResource* spl = gResManager->GetSPL(spell);
	if (spl != NULL) {
		std::string dialog = IDTable::GetDialog(spl->DisplayNameRef());
		if (!dialog.empty())
			name = dialog;
		gResManager->ReleaseResource(spl);
	}
	GUI::Get()->SetHoverTooltip(name);
	return true;
}


/* virtual */
bool
SpellbookScreen::ControlRightClicked(uint16 windowID, uint32 controlID)
{
	if (windowID != kContentWindow)
		return false;
	auto known = fKnown.find(controlID);
	if (known != fKnown.end()) {
		_ShowSpellInfo(known->second);
		return true;
	}
	auto memo = fMemo.find(controlID);
	if (memo != fMemo.end())
		_ShowSpellInfo(memo->second);
	return true;
}


// Populates and shows the spellbook's examine popup (GUIMG/GUIPR window
// 3) for a spell - name + description, same pattern as _ShowItemInfo().
void
SpellbookScreen::_ShowSpellInfo(const res_ref& spellName)
{
	SPLResource* spl = gResManager->GetSPL(spellName);
	if (spl == NULL)
		return;

	GUI::Get()->ShowAuxWindow(CHUName(), kInfoWindow);
	Window* window = GetWindow(kInfoWindow);
	if (window == NULL) {
		gResManager->ReleaseResource(spl);
		return;
	}

	Label* title = dynamic_cast<Label*>(window->GetControlByID(268435455));
	if (title != NULL) {
		std::string name = IDTable::GetDialog(spl->DisplayNameRef());
		title->SetText(name.empty() ? spellName.CString() : name);
	}

	TextArea* description = dynamic_cast<TextArea*>(window->GetControlByID(3));
	if (description != NULL) {
		description->ClearText();
		std::string text = IDTable::GetDialog(spl->DisplayDescriptionRef());
		description->AddText(text.empty() ? "(No description)" : text.c_str());
		description->ScrollTo(0, 0);
	}

	gResManager->ReleaseResource(spl);
}
