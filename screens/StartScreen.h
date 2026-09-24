#pragma once

#include "GameScreen.h"


// The start menu (START.CHU): what the game shows after the intro movies. Two
// pages of the same window: Single Player / Multi Player / Movies / Exit, and
// after Single Player New Game / Load Game / Back. Exit asks for a
// confirmation (window 3). It only records what was chosen (Selected()); the
// front end acts on it. Multi Player, Movies and Options (BG2) aren't there.
class StartScreen : public GameScreen {
public:
	enum Choice { CHOICE_NONE, CHOICE_NEW_GAME, CHOICE_LOAD_GAME, CHOICE_QUIT };

	StartScreen(Game& game);

	virtual void Refresh();
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);

	Choice Selected() const;
	// Back to the first page, nothing chosen.
	void Reset();

private:
	enum Page { PAGE_MAIN, PAGE_SINGLE_PLAYER };

	static const uint16 kMainWindow = 0;
	static const uint16 kQuitWindow = 3;

	void _SetButton(uint32 controlID, uint32 strRef, bool enabled);

	Page fPage;
	Choice fChoice;
};
