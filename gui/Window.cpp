/*
 * Window.cpp
 *
 *  Created on: 18/ott/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "Control.h"
#include "GraphicsEngine.h"
#include "RoomBase.h"
#include "Window.h"

#include "RectUtils.h"
#include "Timer.h"

// Window
Window::Window(uint16 id, int16 xPos, int16 yPos,
		int16 width, int16 height,
		Bitmap* background)
	:
	fID(id),
	fShown(false),
	fBackground(background),
	fWidth(width),
	fHeight(height),
	fLastPulseTime(0),
	fActiveControl(NULL)
{
	fPosition.x = xPos;
	fPosition.y = yPos;
}


Window::~Window()
{
	if (fBackground != NULL)
		fBackground->Release();
	for (std::vector<Control*>::const_iterator i = fControls.begin();
			i != fControls.end(); i++)
		delete *i;

	fControls.clear();
}


void
Window::Add(Control* control)
{
	if (control != NULL) {
		fControls.push_back(control);
		control->AttachedToWindow(this);
	}
}


uint16
Window::ID() const
{
	return fID;
}


void
Window::Show()
{
	fShown = true;
}


void
Window::Hide()
{
	fShown = false;
}


bool
Window::Shown() const
{
	return fShown;
}


IE::point
Window::Position() const
{
	return fPosition;
}


uint16
Window::Width() const
{
	return fWidth;
}


uint16
Window::Height() const
{
	return fHeight;
}


GFX::rect
Window::Frame() const
{
	GFX::rect frame(fPosition.x, fPosition.y,
			fWidth, fHeight);

	return frame;
}


void
Window::MoveTo(uint16 x, uint16 y)
{
	fPosition.x = x;
	fPosition.y = y;
}


void
Window::ResizeTo(uint16 newWidth, uint16 newHeight)
{
	fWidth = newWidth;
	fHeight = newHeight;
}


void
Window::SetFrame(uint16 x, uint16 y, uint16 width, uint16 height)
{
	fWidth = width;
	fHeight = height;
	fPosition.x = x;
	fPosition.y = y;
}


Control*
Window::GetControlByID(uint32 id) const
{
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		if (id == control->ID())
			return control;
	}
	return NULL;
}


Control*
Window::GetGUIControl() const
{
	const char* kGuiCtrlName = "GUICTRL";
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = *i;
		if (control != NULL && control->Type() == IE::CONTROL_BUTTON) {
			IE::button* button = (IE::button*)control->InternalControl();
			if (strcmp(button->image.CString(), kGuiCtrlName) == 0)
				return control;
		}
	}
	return NULL;
}


Control*
Window::ReplaceControl(uint32 id, Control* newControl)
{
	std::vector<Control*>::iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		if (id == control->ID()) {
			GFX::rect controlFrame = control->Frame();
			newControl->InternalControl()->id = id;
			newControl->SetFrame(controlFrame.x, controlFrame.y, controlFrame.w, controlFrame.h);
			newControl->AttachedToWindow(this);
			*i = newControl;
			RoomBase* room = dynamic_cast<RoomBase*>(newControl);
			if (room != NULL)
				newControl->AssociateRoom(room);
			return control;
		}
	}
	return NULL;
}


void
Window::MouseDown(IE::point point)
{
	ConvertFromScreen(point);

	if (Control* control = _ControlAtPoint(point))
		control->MouseDown(point);
}


void
Window::MouseUp(IE::point point)
{
	ConvertFromScreen(point);

	if (Control* control = _ControlAtPoint(point))
		control->MouseUp(point);
}


void
Window::MouseMoved(IE::point point)
{
	ConvertFromScreen(point);

	Control* oldActiveControl = fActiveControl;
	Control* control = _ControlAtPoint(point);

	if (oldActiveControl != control) {
		if (oldActiveControl != NULL)
			oldActiveControl->MouseMoved(point, Control::MOUSE_EXIT);
		if (control != NULL)
			control->MouseMoved(point, Control::MOUSE_ENTER);
	} else {
		if (control != NULL)
			control->MouseMoved(point, Control::MOUSE_INSIDE);
	}
	fActiveControl = control;
}


void
Window::Pulse()
{
	const uint32 pulsePeriod = 100;

	uint32 ticks = Timer::Ticks();
	if (fLastPulseTime + pulsePeriod < ticks) {
		std::vector<Control*>::const_iterator i;
		for (i = fControls.begin(); i != fControls.end(); i++) {
			Control* control = (*i);
			if (control != NULL)
				control->Pulse();
		}
		fLastPulseTime = ticks;
	}
}


void
Window::Draw()
{
	GFX::rect clipRect = Frame();
	GraphicsEngine::Get()->SetClipping(&clipRect);
	if (fBackground != NULL) {
		GFX::rect destRect(fPosition.x, fPosition.y,
						fBackground->Width(), fBackground->Height());
		GraphicsEngine::Get()->BlitToScreen(fBackground, NULL, &destRect);
	}

	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		control->Draw();
	}
	GraphicsEngine::Get()->SetClipping(NULL);
}


void
Window::ConvertToScreen(IE::point& point) const
{
	point.x += fPosition.x;
	point.y += fPosition.y;
}


void
Window::ConvertToScreen(GFX::rect& rect) const
{
	rect.x += fPosition.x;
	rect.y += fPosition.y;
}


void
Window::ConvertFromScreen(IE::point& point) const
{
	point.x -= fPosition.x;
	point.y -= fPosition.y;
}


void
Window::ConvertFromScreen(GFX::rect& rect) const
{
	rect.x -= fPosition.x;
	rect.y -= fPosition.y;
}


void
Window::Print() const
{
	//std::cout << "Window controls:" << std::endl;
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		(*i)->Print();
	}
}


Control*
Window::_ControlAtPoint(IE::point point) const
{
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		if (rect_contains(control->Frame(), point)) {
			return (*i);
		}
	}
	return NULL;
}
