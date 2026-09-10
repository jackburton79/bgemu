/*
 * Control.h
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include "IETypes.h"


namespace GFX {
	class rect;
};

class RoomBase;
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
	int Type() const;

	IE::control* InternalControl() const;

	virtual void Draw();
	virtual void AttachedToWindow(Window* window);
	virtual void DetachedFromWindow(Window* window);
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);
	// Right mouse button pressed over this control. Returns true if the
	// control handled it; false (the default) lets GUI fall back to
	// normal left-button handling so the game world's right-click still
	// works. Only controls that mean something different on right-click
	// (currently just inventory-slot Buttons -> examine) override this.
	virtual bool RightMouseDown(IE::point point);
	virtual void Pulse();

	void Invoke();
	void InvokeRightClick();
	// Tells GUI the pointer entered (inside=true) / left this control -
	// used to drive hover tooltips (e.g. an inventory slot's item name).
	void NotifyHovered(bool inside);

	IE::point Position() const;
	void SetPosition(uint16 x, uint16 y);
	IE::point ScreenPosition() const;
	uint16 Width() const;
	uint16 Height() const;
	GFX::rect Frame() const;
	void SetFrame(uint16 x, uint16 y, uint16 width, uint16 height);

	void ConvertFromScreen(IE::point& point) const;

	::Window* Window() const;

	void Print() const;

	static Control* CreateControl(IE::control* control);

protected:
	::Window* fWindow;
	IE::control* fControl;
};

#endif /* CONTROL_H_ */
