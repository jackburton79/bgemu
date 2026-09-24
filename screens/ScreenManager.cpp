#include "ScreenManager.h"

#include "Button.h"
#include "GUI.h"
#include "Window.h"

#include <strings.h>


static void
_SetButtonToggled(Window* window, uint32 controlID, bool toggled)
{
	if (window == NULL)
		return;
	Button* button = dynamic_cast<Button*>(window->GetControlByID(controlID));
	if (button != NULL)
		button->SetToggled(toggled);
}


ScreenManager::ScreenManager()
{
}


ScreenManager::~ScreenManager()
{
}


void
ScreenManager::Add(GameScreen* screen)
{
	fScreens.emplace_back(screen);
}


const std::vector<std::unique_ptr<GameScreen>>&
ScreenManager::Screens() const
{
	return fScreens;
}


GameScreen*
ScreenManager::Find(const res_ref& chuName) const
{
	for (const auto& screen : fScreens) {
		if (screen->CHUName() == chuName)
			return screen.get();
	}
	return nullptr;
}


void
ScreenManager::Toggle(const char* chuName)
{
	if (GameScreen* screen = Find(res_ref(chuName)))
		screen->Toggle();
}


res_ref
ScreenManager::OpenCHU() const
{
	for (const auto& screen : fScreens) {
		if (screen->IsOpen())
			return screen->CHUName();
	}
	return res_ref("");
}


void
ScreenManager::CloseAllExcept(const res_ref& chuName)
{
	for (const auto& screen : fScreens) {
		if (screen->CHUName() != chuName && screen->IsOpen())
			screen->Close();
	}
}


void
ScreenManager::UpdateCommandBar() const
{
	const res_ref activeCHU = OpenCHU();
	const bool anyOpen = activeCHU != res_ref("");

	Window* hudBar = GUI::Get()->GetWindow(GUI::WINDOW_COMMANDS);
	Window* auxBar = anyOpen ? GUI::Get()->GetAuxWindow(activeCHU, 0) : NULL;
	for (const auto& screen : fScreens) {
		const uint32 buttonID = screen->CommandBarButton();
		if (buttonID == kNoCommandBarButton)
			continue;
		const bool active = anyOpen && screen->CHUName() == activeCHU;
		_SetButtonToggled(hudBar, buttonID, active);
		_SetButtonToggled(auxBar, buttonID, active);
	}
}


void
ScreenManager::ShownCharacterChanged()
{
	for (const auto& screen : fScreens) {
		if (screen->IsOpen())
			screen->OnShownCharacterChanged();
	}
}


bool
ScreenManager::ControlInvoked(const res_ref& chuName, uint16 windowID, uint32 controlID) const
{
	GameScreen* screen = Find(chuName);
	return screen != nullptr && screen->ControlInvoked(windowID, controlID);
}


bool
ScreenManager::ControlRightClicked(const res_ref& chuName, uint16 windowID,
	uint32 controlID) const
{
	GameScreen* screen = Find(chuName);
	return screen != nullptr && screen->ControlRightClicked(windowID, controlID);
}


bool
ScreenManager::ControlHovered(const res_ref& chuName, uint16 windowID, uint32 controlID,
	bool inside) const
{
	GameScreen* screen = Find(chuName);
	return screen != nullptr && screen->ControlHovered(windowID, controlID, inside);
}


bool
ScreenManager::BackgroundClicked(const res_ref& chuName, uint16 windowID) const
{
	GameScreen* screen = Find(chuName);
	return screen != nullptr && screen->BackgroundClicked(windowID);
}


bool
ScreenManager::ControlDoubleClicked(const res_ref& chuName, uint16 windowID,
	uint32 controlID) const
{
	GameScreen* screen = Find(chuName);
	return screen != nullptr && screen->ControlDoubleClicked(windowID, controlID);
}
