/*
 * Control.h
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include "IETypes.h"

#include <map>

class GameMap;
class Window;
class Control {
public:
	enum mouse_transit {
		MOUSE_ENTER,
		MOUSE_INSIDE,
		MOUSE_EXIT
	};

	Control(IE::control* control);
	virtual ~Control();

	uint32 ID() const;

	virtual void Draw();
	virtual void AttachedToWindow(Window* window);
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);

	void Invoke();

	IE::point Position() const;
	IE::point ScreenPosition() const;
	uint16 Width() const;
	uint16 Height() const;

	void ConvertFromScreen(IE::point& point);

	void AssociateRoom(GameMap* room);
	void Print() const;

	static Control* CreateControl(IE::control* control);

protected:
	Window* fWindow;
	IE::control* fControl;
	GameMap* fRoom;
};

#endif /* CONTROL_H_ */
