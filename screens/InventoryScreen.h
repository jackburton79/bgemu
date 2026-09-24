#pragma once

#include "PanelScreen.h"


class Actor;


// The inventory (GUIINV): the shown character's items in their slots, the
// paperdoll, name/class/AC/HP/gold/weight, and the ground pile at their
// feet. Items move with a click-to-pick, click-to-place (or a real
// press-drag-release) between slots; an item held on the cursor is dropped
// to the floor by a click on the background or the ground slots. A right
// click examines an item (window 5, a popup).
class InventoryScreen : public PanelScreen {
public:
	InventoryScreen(Game& game);

	virtual uint32 CommandBarButton() const;
	virtual bool ControlInvoked(uint16 windowID, uint32 controlID);
	virtual bool ControlRightClicked(uint16 windowID, uint32 controlID);
	virtual bool ControlHovered(uint16 windowID, uint32 controlID, bool inside);
	virtual bool BackgroundClicked(uint16 windowID);

protected:
	virtual void OnOpen();
	virtual void OnClose();
	virtual void RefreshContent();
	virtual void PanelControlInvoked(uint16 windowID, uint32 controlID);

private:
	static const uint16 kInfoWindow = 5;

	// A half-finished drag doesn't survive the screen closing, opening or
	// being navigated away from: whatever is on the cursor goes back where
	// it came from.
	void _ClearDrag();
	void _ShowItemInfo(const res_ref& itemName);
	void _DropHeldItemOnGround();
	void _SetSlotIcon(Window* window, Actor* actor, uint32 controlID, uint32 creSlot);
	void _UpdateGroundItemSlots(Window* window, Actor* actor);
	void _UpdatePaperdoll(Window* window, Actor* actor);
	void _UpdateLabels(Window* window, Actor* actor);

	// The CRE slot of the item being dragged, -1 if none.
	int32 fDragSlot;
};
