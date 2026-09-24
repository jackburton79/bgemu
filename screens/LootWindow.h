#pragma once

#include "IETypes.h"

#include <vector>

class Actor;
class Object;


// The loot window (GUIW window 8, in the HUD's own resource rather than an
// auxiliary CHU): while it is up it replaces the message area and command
// bar at the bottom of the screen. `looter` is the party member using
// `source`, either a Container or a dead Actor (corpse); the window shows
// the source's items on the left and the looter's own carried items on the
// right, and a click moves an item across. Opened by the USECONTAINER
// action, closed by "Done" or when the area unloads.
class LootWindow {
public:
	LootWindow();

	void Open(Actor* looter, Object* source);
	void Close();
	bool IsOpen() const;

	void ControlInvoked(uint32 controlID);
	void ControlHovered(uint32 controlID, bool inside);

private:
	// Re-populates the window from the current contents of the source and
	// of the looter's own inventory (clamping the scroll positions first -
	// a move can leave the list shorter than the row being shown).
	void _Refresh();

	Object* fSource;
	Actor* fLooter;
	int32 fLeftRow;
	int32 fRightRow;
	// HUD windows hidden for as long as the loot window is up, so Close()
	// shows back exactly those.
	std::vector<uint16> fHiddenWindows;
};
