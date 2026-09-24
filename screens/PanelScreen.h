#pragma once

#include "GameScreen.h"


// The full-screen panels (Inventory, Record, Journal, Spellbook): window 2
// is the content, flanked by the two windows every one of them shares -
// window 0 with a copy of the command bar and window 1 with the portrait
// column that picks the character shown. Those two are handled here; a
// subclass fills window 2 (RefreshContent()) and handles its controls
// (PanelControlInvoked()).
class PanelScreen : public GameScreen {
public:
	PanelScreen(Game& game, const char* chuName);

	virtual void Refresh();
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);

protected:
	static const uint16 kContentWindow = 2;
	static const uint16 kCommandBarWindow = 0;
	static const uint16 kPortraitWindow = 1;

	virtual void RefreshContent() = 0;
	virtual void PanelControlInvoked(uint16 windowID, uint32 controlID);

private:
	// Party members the portrait column has room for.
	static const uint32 kPortraitCount = 4;
};
