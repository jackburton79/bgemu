#include "GameScreen.h"

#include "Game.h"
#include "GUI.h"


GameScreen::GameScreen(Game& game, const char* chuName, std::vector<uint16> windows)
	:
	fGame(game),
	fCHU(chuName),
	fWindows(std::move(windows))
{
}


/* virtual */
GameScreen::~GameScreen()
{
}


const res_ref&
GameScreen::CHUName() const
{
	return fCHU;
}


bool
GameScreen::IsOpen() const
{
	return GUI::Get() != NULL && GUI::Get()->IsAuxWindowShown(fCHU, fWindows.front());
}


void
GameScreen::Open()
{
	fGame.CloseOtherScreens(fCHU.CString());

	// Hidden first, then shown in order, so they end up stacked in that
	// order however they were before.
	for (uint16 windowID : fWindows)
		GUI::Get()->HideAuxWindow(fCHU, windowID);
	for (uint16 windowID : fWindows)
		GUI::Get()->ShowAuxWindow(fCHU, windowID);

	OnOpen();
	Refresh();
	fGame.UpdateCommandBarToggle();
}


void
GameScreen::Close()
{
	for (uint16 windowID : fWindows)
		GUI::Get()->HideAuxWindow(fCHU, windowID);

	OnClose();
	fGame.UpdateCommandBarToggle();
}


void
GameScreen::Toggle()
{
	if (IsOpen())
		Close();
	else
		Open();
}


/* virtual */
void
GameScreen::OnShownCharacterChanged()
{
	Refresh();
}


/* virtual */
uint32
GameScreen::CommandBarButton() const
{
	return kNoCommandBarButton;
}


/* virtual */
bool
GameScreen::ControlInvoked(uint16 /*windowID*/, uint32 /*controlID*/)
{
	return false;
}


/* virtual */
bool
GameScreen::ControlRightClicked(uint16 /*windowID*/, uint32 /*controlID*/)
{
	return false;
}


/* virtual */
bool
GameScreen::ControlHovered(uint16 /*windowID*/, uint32 /*controlID*/, bool /*inside*/)
{
	return false;
}


/* virtual */
bool
GameScreen::ControlDoubleClicked(uint16 /*windowID*/, uint32 /*controlID*/)
{
	return false;
}


/* virtual */
bool
GameScreen::BackgroundClicked(uint16 /*windowID*/)
{
	return false;
}


/* virtual */
void
GameScreen::OnOpen()
{
}


/* virtual */
void
GameScreen::OnClose()
{
}


Window*
GameScreen::GetWindow(uint16 windowID) const
{
	return GUI::Get()->GetAuxWindow(fCHU, windowID);
}
