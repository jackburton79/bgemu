#include "PanelScreen.h"

#include "CommandBar.h"
#include "Game.h"


PanelScreen::PanelScreen(Game& game, const char* chuName)
	:
	GameScreen(game, chuName, { kContentWindow, kCommandBarWindow, kPortraitWindow })
{
}


/* virtual */
void
PanelScreen::Refresh()
{
	fGame.UpdatePortraitColumn(GetWindow(kPortraitWindow), kPortraitCount);
	RefreshContent();
}


/* virtual */
bool
PanelScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kCommandBarWindow)
		CommandBar::InvokeInPanel(controlID);
	else if (windowID == kPortraitWindow) {
		if (controlID < kPortraitCount)
			fGame.ShowCharacter(static_cast<uint16>(controlID));
	} else
		PanelControlInvoked(windowID, controlID);
	return true;
}


/* virtual */
void
PanelScreen::PanelControlInvoked(uint16 /*windowID*/, uint32 /*controlID*/)
{
}
