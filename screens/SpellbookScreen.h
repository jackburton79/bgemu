#pragma once

#include "PanelScreen.h"

#include <map>


// The spellbook (GUIMG for arcane spells, GUIPR for divine ones - one
// instance each): the shown character's known spells (right page) and
// memorized ones (left page) of one spell level, memorized/released with a
// click, described by a right click (window 3, a popup).
class SpellbookScreen : public PanelScreen {
public:
	SpellbookScreen(Game& game, bool divine);

	virtual uint32 CommandBarButton() const;
	virtual bool ControlRightClicked(uint16 windowID, uint32 controlID);
	virtual bool ControlHovered(uint16 windowID, uint32 controlID, bool inside);

protected:
	virtual void OnOpen();
	virtual void OnClose();
	virtual void RefreshContent();
	virtual void PanelControlInvoked(uint16 windowID, uint32 controlID);

private:
	static const uint16 kInfoWindow = 3;

	void _ShowSpellInfo(const res_ref& spellName);

	bool fDivine;
	uint16 fLevel;
	// Grid control id -> the spell it currently shows.
	std::map<uint32, res_ref> fKnown;
	std::map<uint32, res_ref> fMemo;
};
