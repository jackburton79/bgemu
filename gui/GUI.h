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
	GUI(const res_ref& name);
	~GUI();

	void Draw();

	void MouseDown(int16 x, int16 y);
	void MouseUp(int16 x, int16 y);
	void MouseMoved(int16 x, int16 y);

private:
	CHUIResource* fResource;
	std::vector<Window*> fWindows;

	Window* _GetWindow(IE::point point);
};

#endif /* __GUI_H_ */
