#include "StartScreen.h"

#include "Button.h"
#include "Game.h"
#include "GUI.h"
#include "ResManager.h"
#include "TextArea.h"
#include "Window.h"

// Control ids of START.CHU's main window, the same in both games (BG2 also has
// Options and Back, unused here), and their captions as GemRB's Start.py sets
// them - the strrefs of the loaded TLK, so in its language.
static const uint32 kSinglePlayerButton = 0;
static const uint32 kMultiPlayerButton = 1;
static const uint32 kMoviesButton = 2;
static const uint32 kExitButton = 3;
static const uint32 kOptionsButton = 4;
static const uint32 kBackButton = 5;

static const uint32 kSinglePlayerStrRef = 15413;
static const uint32 kMultiPlayerStrRef = 15414;
static const uint32 kMoviesStrRef = 15415;
static const uint32 kExitStrRef = 15417;
static const uint32 kNewGameStrRef = 13728;
static const uint32 kLoadGameStrRef = 13729;
static const uint32 kBackStrRef = 15416;

// The confirmation window: text, Quit, Cancel.
static const uint32 kQuitTextControl = 0;
static const uint32 kQuitConfirmControl = 1;
static const uint32 kQuitCancelControl = 2;
static const uint32 kQuitTextStrRef = 19532;
static const uint32 kQuitCancelStrRef = 13727;


StartScreen::StartScreen(Game& game)
	:
	GameScreen(game, "START", { kMainWindow }),
	fPage(PAGE_MAIN),
	fChoice(CHOICE_NONE)
{
}


void
StartScreen::_SetButton(uint32 controlID, uint32 strRef, bool enabled)
{
	Window* window = GUI::Get()->GetAuxWindow(CHUName(), kMainWindow);
	Button* button = window != NULL
		? dynamic_cast<Button*>(window->GetControlByID(controlID)) : NULL;
	if (button == NULL)
		return;
	button->SetText(strRef != 0 ? IDTable::GetDialog(strRef) : "");
	button->SetEnabled(enabled);
	// A button with nothing to say isn't drawn at all.
	button->SetFrameless(strRef == 0);
}


/* virtual */
void
StartScreen::Refresh()
{
	if (fPage == PAGE_MAIN) {
		_SetButton(kSinglePlayerButton, kSinglePlayerStrRef, true);
		_SetButton(kMultiPlayerButton, kMultiPlayerStrRef, false);
		_SetButton(kMoviesButton, kMoviesStrRef, false);
		_SetButton(kExitButton, kExitStrRef, true);
	} else {
		_SetButton(kSinglePlayerButton, kNewGameStrRef, true);
		_SetButton(kMultiPlayerButton, kLoadGameStrRef, true);
		_SetButton(kMoviesButton, 0, false);
		_SetButton(kExitButton, kBackStrRef, true);
	}
	_SetButton(kOptionsButton, 0, false);
	_SetButton(kBackButton, 0, false);
}


/* virtual */
bool
StartScreen::ControlInvoked(uint16 windowID, uint32 controlID)
{
	if (windowID == kQuitWindow) {
		if (controlID == kQuitConfirmControl)
			fChoice = CHOICE_QUIT;
		else if (controlID == kQuitCancelControl)
			GUI::Get()->HideAuxWindow(CHUName(), kQuitWindow);
		return true;
	}
	if (windowID != kMainWindow)
		return false;

	if (fPage == PAGE_MAIN) {
		if (controlID == kSinglePlayerButton) {
			fPage = PAGE_SINGLE_PLAYER;
			Refresh();
		} else if (controlID == kExitButton) {
			GUI::Get()->ShowAuxWindow(CHUName(), kQuitWindow);
			if (Window* window = GUI::Get()->GetAuxWindow(CHUName(), kQuitWindow)) {
				if (TextArea* text = dynamic_cast<TextArea*>(window->GetControlByID(kQuitTextControl))) {
					text->ClearText();
					text->AddText(IDTable::GetDialog(kQuitTextStrRef).c_str());
				}
				if (Button* confirm = dynamic_cast<Button*>(window->GetControlByID(kQuitConfirmControl)))
					confirm->SetText(IDTable::GetDialog(kExitStrRef));
				if (Button* cancel = dynamic_cast<Button*>(window->GetControlByID(kQuitCancelControl)))
					cancel->SetText(IDTable::GetDialog(kQuitCancelStrRef));
			}
		}
	} else {
		if (controlID == kSinglePlayerButton)
			fChoice = CHOICE_NEW_GAME;
		else if (controlID == kMultiPlayerButton)
			fChoice = CHOICE_LOAD_GAME;
		else if (controlID == kExitButton) {
			fPage = PAGE_MAIN;
			Refresh();
		}
	}
	return true;
}


StartScreen::Choice
StartScreen::Selected() const
{
	return fChoice;
}


void
StartScreen::Reset()
{
	fPage = PAGE_MAIN;
	fChoice = CHOICE_NONE;
}
