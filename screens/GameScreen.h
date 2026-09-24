#pragma once

#include "IETypes.h"

#include <vector>

class Game;
class Window;

// Command-bar id of a screen that has no icon on it.
constexpr uint32 kNoCommandBarButton = static_cast<uint32>(-1);


// A screen of the game (Inventory, Record, ...) filled in from a CHU
// resource: one CHU, made of one or more of its windows shown and hidden
// together. All of its state that must not outlive the windows lives in the
// windows themselves, so a screen is "open" exactly when its first window
// is shown - and closes on its own when GUI::Clear() destroys them.
//
// Opening one closes whichever other screen is open (ScreenManager::
// CloseAllExcept()) and refreshes the command-bar icon that shows the
// open screen; subclasses fill in the content.
class GameScreen {
public:
	GameScreen(Game& game, const char* chuName, std::vector<uint16> windows);
	virtual ~GameScreen();

	const res_ref& CHUName() const;
	virtual bool IsOpen() const;
	void Open();
	void Close();
	void Toggle();

	// Fills the screen's controls from the current game state.
	virtual void Refresh() = 0;
	// The character the character screens show has changed; only called
	// while the screen is open.
	virtual void OnShownCharacterChanged();
	// Control id of this screen's icon on the command bar.
	virtual uint32 CommandBarButton() const;

	// Events of the screen's own windows. Each returns whether the screen
	// dealt with it.
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);
	virtual bool ControlRightClicked(uint16 windowID, uint32 controlID);
	virtual bool ControlHovered(uint16 windowID, uint32 controlID, bool inside);
	virtual bool ControlDoubleClicked(uint16 windowID, uint32 controlID);
	// A click on one of the screen's windows that landed on no control.
	virtual bool BackgroundClicked(uint16 windowID);

protected:
	// Called just after the windows are shown, before Refresh(), and just
	// after they are hidden.
	virtual void OnOpen();
	virtual void OnClose();

	Window* GetWindow(uint16 windowID) const;

	Game& fGame;

private:
	res_ref fCHU;
	std::vector<uint16> fWindows;
};
