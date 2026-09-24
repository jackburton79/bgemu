#pragma once

#include "GameScreen.h"

#include <memory>
#include <vector>


// The game's screens: which ones exist, and the events that GUI hands to
// whichever one owns the CHU a control belongs to.
class ScreenManager {
public:
	ScreenManager();
	~ScreenManager();

	// Takes ownership.
	void Add(GameScreen* screen);

	const std::vector<std::unique_ptr<GameScreen>>& Screens() const;
	GameScreen* Find(const res_ref& chuName) const;

	template <class T>
	T* Find() const
	{
		for (const auto& screen : fScreens) {
			if (T* found = dynamic_cast<T*>(screen.get()))
				return found;
		}
		return nullptr;
	}

	template <class T>
	void Toggle()
	{
		if (T* screen = Find<T>())
			screen->Toggle();
	}

	// For screens that share a class (Save and Load) and so can't be told
	// apart by type.
	void Toggle(const char* chuName);

	// The CHU of the open screen, empty if none is.
	res_ref OpenCHU() const;
	void CloseAllExcept(const res_ref& chuName);
	void ShownCharacterChanged();

	bool ControlInvoked(const res_ref& chuName, uint16 windowID, uint32 controlID) const;
	bool ControlRightClicked(const res_ref& chuName, uint16 windowID, uint32 controlID) const;
	bool ControlHovered(const res_ref& chuName, uint16 windowID, uint32 controlID,
		bool inside) const;

private:
	std::vector<std::unique_ptr<GameScreen>> fScreens;
};
