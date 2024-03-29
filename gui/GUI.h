/*
 * GUI.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef __GUI_H_
#define __GUI_H_

#include "GraphicsDefs.h"
#include "IETypes.h"
#include "Listener.h"
#include "Window.h"

#include <map>
#include <list>
#include <string>
#include <vector>

class Bitmap;
namespace GFX {
	struct rect;
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
	void MouseMoved(int16 x, int16 y);

	void GetCursorPosition(int16& x, int16& y) const;

	void ShowWindow(uint16 id);
	void HideWindow(uint16 id);
	bool IsWindowShown(uint16 id) const;
	void ToggleWindow(uint16 id);

	void AddWindow(Window* window);
	Window* GetWindow(uint16 id) const;
	TextArea* GetMessagesTextArea();
	void EnsureShowDialogArea();
	void ToggleMessageArea();

	void SetArrowCursor(uint32 index);
	void SetCursor(uint32 index);

	void UpdateCursorAndScrolling(int x, int y);

	void ControlInvoked(uint32 controlID, uint16 windowID);

	void RemoveToolTip(uint32 id);

	static GUI* Get();

private:
	CHUIResource* fResource;
	std::vector<Window*> fWindows;
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
	
	GUI(uint16 width, uint16 height);
	~GUI();
	
	Window* _WindowAtPoint(IE::point point);
	void _AddBackgroundWindow();
	void _InitCursors();
	void _DrawStrings();
	void _DisplayStringCommon(const std::string& text,
			uint16 x, uint16 y, bool centerString, uint32 time);
};

#endif /* __GUI_H_ */
