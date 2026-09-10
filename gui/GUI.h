/*
 * GUI.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#pragma once

#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Listener.h"
#include "Window.h"

#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Bitmap;
namespace GFX {
	class rect;
};

struct string_entry {
	std::string text;
	Bitmap* bitmap;
	GFX::rect rect;
	uint32 id;
};

class Animation;
class BAMResource;
class CHUIResource;
class TextArea;
class GUI : public Listener {
public:
	enum {
		WINDOW_COMMANDS = 0,
		WINDOW_PLAYER_SLOTS = 1,
		WINDOW_CMDS = 3,
		WINDOW_MESSAGES = 4,
		WINDOW_MESSAGES_LARGE = 7
	};

	static bool Initialize(const uint16 width, const uint16 height);
	static void Destroy();

	bool Load(const res_ref& name);
	void Clear();

	void Hide();
	void Show();

	void Draw();
	void DisplayString(const std::string& text,
			uint16 x, uint16 y, uint32 time);
	void DisplayStringCentered(const std::string& text,
			uint16 xCenter, uint16 yCenter, uint32 time);

	void MouseDown(int16 x, int16 y);
	void MouseUp(int16 x, int16 y);
	// Right button pressed: offered to the topmost window's control at
	// (x,y); if nothing consumes it, falls back to MouseDown() so the
	// game world's right-click keeps working.
	void RightMouseDown(int16 x, int16 y);
	void MouseMoved(int16 x, int16 y);

	void GetCursorPosition(int16& x, int16& y) const;

	void ShowWindow(uint16 id);
	void HideWindow(uint16 id);
	bool IsWindowShown(uint16 id) const;
	void ToggleWindow(uint16 id);

	void AddWindow(Window* window);
	Window* GetWindow(uint16 id) const;

	// Auxiliary screens (Inventory, Record, Spellbook, ...): each one
	// lives in its own CHU resource, separate from the main GUIW/GUIWMAP
	// resource loaded via Load()/fResource above - GetWindow(id)'s single
	// flat id lookup isn't safe to reuse here since two different CHU
	// files can (and typically do) both define e.g. a window id 0.
	// Windows opened this way still draw/receive mouse events normally
	// (they're kept in the same fWindows list as everything else) - this
	// is only a separate (chuName, windowId) -> Window* lookup so
	// repeated Show calls reuse the same instance instead of rebuilding
	// it from the CHU every time.
	bool LoadAuxiliary(const res_ref& chuName);
	void ShowAuxWindow(const res_ref& chuName, uint16 windowId);
	void HideAuxWindow(const res_ref& chuName, uint16 windowId);
	bool IsAuxWindowShown(const res_ref& chuName, uint16 windowId) const;
	void ToggleAuxWindow(const res_ref& chuName, uint16 windowId);
	// Show/hide, as a unit, every window of a multi-window aux screen
	// (e.g. Inventory or Record: a main content panel plus the two
	// persistent side columns) - shown state is read from windowIds's
	// first entry and applied to all of them. Returns true if the group
	// is now shown (false if it just got hidden), so a caller that needs
	// to populate the content only on open (e.g. with per-character data)
	// knows when to do so.
	bool ToggleAuxWindowGroup(const res_ref& chuName,
		std::initializer_list<uint16> windowIds);
	// NULL if that (chuName, windowId) hasn't been shown yet - unlike
	// GetWindow(id) above, this one does look inside fAuxWindows (by
	// design GetWindow() excludes aux windows, since their plain numeric
	// id can collide across different CHU files). Lets callers outside
	// GUI (e.g. Game, to populate inventory slot icons) reach a specific
	// aux window's controls.
	Window* GetAuxWindow(const res_ref& chuName, uint16 windowId) const;

	TextArea* GetMessagesTextArea();
	void EnsureShowDialogArea();
	void ToggleMessageArea();

	void SetArrowCursor(uint32 index);
	void SetCursor(uint32 index);

	void UpdateCursorAndScrolling(int x, int y);

	// chuName - which CHU resource the clicked window was loaded from
	// (see Window::OwnerCHU()) - empty for this GUI's own primary
	// resource (fResource), non-empty for an aux screen (GUIINV,
	// GUIREC, GUISAVE, ...). Needed because window/control ids aren't
	// unique across different CHU files, only within one.
	void ControlInvoked(uint32 controlID, uint16 windowID, const res_ref& chuName);
	// Right-click on a control the control itself chose to forward (see
	// Control::RightMouseDown) - currently inventory-slot Buttons only.
	void ControlRightClicked(uint32 controlID, uint16 windowID, const res_ref& chuName);
	// Pointer entered (inside=true) / left a control - drives hover
	// tooltips.
	void ControlHovered(uint32 controlID, uint16 windowID, const res_ref& chuName,
						bool inside);

	// Small text shown next to the cursor while hovering something (an
	// inventory slot's item name). Empty string clears it.
	void SetHoverTooltip(const std::string& text);

	void RemoveToolTip(uint32 id);

	// An item icon that follows the cursor while the player is dragging
	// an inventory item between slots (see Game::InventoryControlInvoked()).
	// Takes ownership of the passed reference; NULL clears it. Purely a
	// render hook - the drag's actual model (which actor, which slot)
	// lives in Game.
	void SetDragBitmap(Bitmap* bitmap);
	bool IsDraggingItem() const;

	static GUI* Get();

private:
	CHUIResource* fResource;
	std::vector<Window*> fWindows;
	std::map<res_ref, CHUIResource*> fAuxResources;
	std::map<std::pair<res_ref, uint16>, Window*> fAuxWindows;
	// Every Window* also present in fAuxWindows above - lets GetWindow(id)
	// (and everything built on it: ShowWindow/HideWindow/IsWindowShown/
	// ToggleWindow) skip aux windows, since their ids are only unique
	// within their own CHU, not against the main fResource's ids (e.g.
	// GUIINV's window 0 and GUIW's WINDOW_COMMANDS=0 are unrelated
	// windows that happen to share an id).
	std::set<Window*> fAuxWindowSet;
	Window* fBackWindow;
	Animation* fCursors[NUM_CURSORS];
	Animation* fCurrentCursor;
	IE::point fCursorPosition;
	std::list<string_entry> fTooltipList;

	uint16 fScreenWidth;
	uint16 fScreenHeight;

	uint32 fLastScrollTime;
	bool fShown;

	Bitmap* fTooltipBitmap;
	Bitmap* fDragBitmap;
	Bitmap* fHoverTooltipBitmap;
	std::string fHoverTooltipText;
	// Window that grabbed the mouse in its last MouseDown (a control
	// inside it is being dragged) - MouseMoved/MouseUp go here until the
	// button is released, so a drag survives the cursor leaving it.
	Window* fCaptureWindow;

	GUI(uint16 width, uint16 height);
	~GUI();

	Window* _WindowAtPoint(IE::point point);
	void _AddBackgroundWindow();
	void _CenterWindow(Window* window, const std::string& chuName) const;
	void _InitCursors();
	void _DrawStrings();
	void _DisplayStringCommon(const std::string& text,
			uint16 x, uint16 y, bool centerString, uint32 time);
};
