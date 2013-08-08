/*
 * Window.cpp
 *
 *  Created on: 18/ott/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "Control.h"
#include "GraphicsEngine.h"
#include "Window.h"


// Window
Window::Window(uint16 id, int16 xPos, int16 yPos,
		int16 width, int16 height,
		Bitmap* background)
	:
	fID(id),
	fBackground(background),
	fWidth(width),
	fHeight(height),
	fActiveControl(NULL)
{
	fPosition.x = xPos;
	fPosition.y = yPos;

	// TODO: Not very nice. Here we check if the window would sit
	// on the right side on the screen on 640x480, and move it if
	// the screen is larger than that. We also check if the window would
	// span into the full width of the screen, and resize it accordingly.
	GFX::rect screenRect = GraphicsEngine::Get()->VideoArea();
	if (screenRect.w > 640) {
		if (xPos + width == 640) {
			if (xPos == 0) {
				// Full screen window. Resize
				SetFrame(xPos, yPos, screenRect.w, screenRect.h);
			} else {
				SetFrame(screenRect.w - width, yPos, width, height);
			}
		}
	}
}


Window::~Window()
{
	GraphicsEngine::DeleteBitmap(fBackground);
	fControls.erase(fControls.begin(), fControls.end());
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


void
Window::SetFrame(uint16 x, uint16 y, uint16 width, uint16 height)
{
	fWidth = width;
	fHeight = height;
	fPosition.x = x;
	fPosition.y = y;

	//TODO: Don't hardcode here. Move to BackWindow ?
	Control* control = GetControlByID(uint32(-1));
	if (control != NULL)
		control->SetFrame(0, 0, width, height);
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


void
Window::MouseDown(IE::point point)
{
	point.x -= fPosition.x;
	point.y -= fPosition.y;

	if (Control* control = _GetControl(point)) {
		control->MouseDown(point);
	}
}


void
Window::MouseUp(IE::point point)
{
	point.x -= fPosition.x;
	point.y -= fPosition.y;

	if (Control* control = _GetControl(point)) {
		control->MouseUp(point);
	}
}


void
Window::MouseMoved(IE::point point)
{
	point.x -= fPosition.x;
	point.y -= fPosition.y;

	Control* oldActiveControl = fActiveControl;
	Control* control = _GetControl(point);

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
Window::Draw()
{
	if (fBackground != NULL) {
		GFX::rect destRect = {
				fPosition.x,
				fPosition.y,
				fBackground->Width(),
				fBackground->Height()
		};
		GraphicsEngine::Get()->BlitToScreen(fBackground, NULL, &destRect);
	}

	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		control->Draw();
	}
}


void
Window::ConvertToScreen(IE::point& point)
{
	point.x += fPosition.x;
	point.y += fPosition.y;
}


void
Window::ConvertToScreen(GFX::rect& rect)
{
	rect.x += fPosition.x;
	rect.y += fPosition.y;
}


void
Window::Print() const
{
	std::cout << "Window controls:" << std::endl;
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		//(*i)->Print();
	}
}


Control*
Window::_GetControl(IE::point point)
{
	std::vector<Control*>::const_iterator i;
	for (i = fControls.begin(); i != fControls.end(); i++) {
		Control* control = (*i);
		const IE::point pt = control->Position();
		if (point.x >= pt.x && point.x <= pt.x + control->Width()
				&& point.y >= pt.y && point.y <= pt.y + control->Height()) {
			return (*i);
		}
	}
	return NULL;
}
