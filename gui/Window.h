/*
 * Window.h
 *
 *  Created on: 18/ott/2012
 *      Author: stefano
 */

#ifndef __WINDOW_H_
#define __WINDOW_H_

#include <vector>

#include "IETypes.h"
#include "SupportDefs.h"

class Bitmap;
class CHUIResource;
class Control;
namespace GFX {
	class rect;
	class point;
}
class Window {
public:
	// ownerCHU - the CHU resource this window was loaded from (e.g.
	// "GUIINV", "GUISAVE") - lets a click on one of its controls
	// (Control::Invoke() -> GUI::ControlInvoked()) be routed to the
	// right screen's handling even for aux windows, whose numeric
	// window/control ids aren't unique across different CHU files.
	Window(uint16 id, int16 xPos, int16 yPos, int16 width, int16 height,
			Bitmap* background, const res_ref& ownerCHU = res_ref());
	~Window();

	const res_ref& OwnerCHU() const { return fOwnerCHU; }

	void Draw();
	void Add(Control* control);

	uint16 ID() const;

	void Show();
	void Hide();
	bool Shown() const;

	GFX::rect Frame() const;
	IE::point Position() const;

	uint16 Width() const;
	uint16 Height() const;

	void MoveTo(uint16 x, uint16 y);
	void ResizeTo(uint16 newWidth, uint16 newHeight);
	void SetFrame(uint16 x, uint16 y, uint16 width, uint16 height);
	void SetFrame(const GFX::rect rect);

	Control* GetControlByID(uint32 id) const;
	Control* GetGUIControl() const;

	Control* ReplaceControl(uint32 id, Control* newcontrol);

	void MouseDown(IE::point point);
	void MouseUp(IE::point point);
	void MouseMoved(IE::point point);

	void Pulse();

	void ConvertToScreen(IE::point& point) const;
	void ConvertToScreen(GFX::rect& rect) const;

	void ConvertFromScreen(IE::point& point) const;
	void ConvertFromScreen(GFX::rect& rect) const;

	void Print() const;

private:
	Control* _ControlAtPoint(IE::point point) const;

	uint16 fID;
	bool fShown;
	Bitmap* fBackground;
	std::vector<Control*> fControls;
	IE::point fPosition;
	uint16 fWidth;
	uint16 fHeight;
	uint32 fLastPulseTime;
	Control* fActiveControl;
	res_ref fOwnerCHU;
};


#endif /* WINDOW_H_ */
