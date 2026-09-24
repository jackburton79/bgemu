#include "ScreenManager.h"


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
