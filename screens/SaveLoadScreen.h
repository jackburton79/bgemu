#pragma once

#include "GameScreen.h"


// The Save and Load screens (GUISAVE / GUILOAD, one instance each): a
// standalone full-screen window (no side columns, unlike the panels) with 4
// slot rows, each a name, a date, a Save-or-Load button and a Delete
// button. Real BG2's scrolling past 4 saves, name-entry popup and per-row
// thumbnail/portraits aren't supported - this engine's saves have no name,
// timestamp or preview of their own to show (see GamResource's header
// comment), and neither TextEdit nor Scrollbar support what a real
// name-entry field or scrolling list needs yet. The saving itself is Game's
// (Game::Save()/Load()).
class SaveLoadScreen : public GameScreen {
public:
	SaveLoadScreen(Game& game, bool save);

	// Fills every slot row's name/date labels and Save-or-Load/Delete
	// button state (enabled iff that slot's own .gam exists - Delete's
	// case - or, for a Load screen's own action button, iff it exists at
	// all; a Save screen's own action button stays enabled on an empty
	// row too, since saving into one is how a new save is made).
	virtual void Refresh();
	virtual uint32 CommandBarButton() const;
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);

private:
	static const uint16 kWindow = 0;

	bool fIsSave;
};
