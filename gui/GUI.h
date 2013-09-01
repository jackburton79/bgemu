/*
 * GUI.h
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#ifndef __GUI_H_
#define __GUI_H_

#include "IETypes.h"
#include "Window.h"

#include <map>
#include <vector>

class Animation;
class CHUIResource;
class GUI {
public:
	GUI();
	~GUI();

	void Load(const res_ref& name);
	void Clear();

	void Draw();

	void MouseDown(int16 x, int16 y);
	void MouseUp(int16 x, int16 y);
	void MouseMoved(int16 x, int16 y);

	void ShowWindow(uint16 id);
	void HideWindow(uint16 id);
	bool IsWindowShown(uint16 id) const;
	void ToggleWindow(uint16 id);

	Window* GetWindow(uint16 id);
	void SetArrowCursor(uint32 index);
	void SetCursor(uint32 index);

	void ControlInvoked(uint32 controlID, uint16 windowID);

	static GUI* Get();
	static void Destroy();

private:
	CHUIResource* fResource;
	std::vector<Window*> fActiveWindows;
	Animation* fCursors[NUM_CURSORS];
	Animation* fCurrentCursor;
	IE::point fCursorPosition;

	Window* _GetWindow(IE::point point);
	void _AddBackgroundWindow();
	void _InitCursors();
};

#endif /* __GUI_H_ */
