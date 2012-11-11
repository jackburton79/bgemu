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

#include <vector>

class CHUIResource;
class GUI {
public:
	GUI();
	~GUI();

	void Load(const res_ref& name);
	void Draw();

	void MouseDown(int16 x, int16 y);
	void MouseUp(int16 x, int16 y);
	void MouseMoved(int16 x, int16 y);

	void Clear();
	Window* GetWindow(uint32 id);

	void ControlInvoked(uint32 controlID, uint16 windowID);

	static GUI* Default();

private:
	CHUIResource* fResource;
	std::vector<Window*> fActiveWindows;

	Window* _GetWindow(IE::point point);
};

#endif /* __GUI_H_ */
