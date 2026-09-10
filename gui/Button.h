/*
 * Button.h
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#ifndef __BUTTON_H_
#define __BUTTON_H_

#include "Control.h"
#include "GraphicsDefs.h"
#include "IETypes.h"

class Bitmap;
class Window;
class Button: public Control {
public:
	Button(IE::button* button);
	virtual ~Button();
	virtual void AttachedToWindow(::Window* window);
	virtual void Draw();
	virtual void MouseMoved(IE::point point, uint32 transit);
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);
	virtual bool RightMouseDown(IE::point point);

	// Overlay drawn centered on top of the button's own frame - e.g. an
	// item icon over an inventory slot's empty-slot background. Takes a
	// reference (like the CHU-authored bitmaps above); pass NULL to clear
	// it. Not part of the CHU format itself, so no constructor-time
	// equivalent. With coverBackground the button's own frame bitmap is
	// hidden while the icon is set (for the paperdoll, whose CHU frame is
	// just a generic placeholder doll the real PLT fully replaces).
	void SetIcon(Bitmap* icon, bool coverBackground = false);

	// Draws a bright outline around the button - used to mark the
	// currently-selected party member's portrait.
	void SetHighlighted(bool highlighted);

private:
	Bitmap* fDisabledBitmap;
	Bitmap* fSelectedBitmap;
	Bitmap* fPressedBitmap;
	Bitmap* fUnpressedBitmap;
	Bitmap* fIcon;
	// Icon position/size within the button's own (window-local, not yet
	// screen-converted) frame - computed once in SetIcon() rather than
	// every Draw() call, since it only depends on the icon's fixed size
	// and the button's own (static, CHU-authored) frame, neither of
	// which change between frames.
	GFX::rect fIconRect;
	bool fCoverBackground;
	bool fHighlighted;
	bool fEnabled;
	bool fSelected;
	bool fPressed;
};

#endif /* BUTTON_H_ */
