/*
 * Control.cpp
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#include "Button.h"
#include "GraphicsDefs.h"

#include "Control.h"
#include "GUI.h"
#include "Label.h"
#include "Room.h"
#include "Scrollbar.h"
#include "Slider.h"
#include "TextArea.h"
#include "TextEdit.h"
#include "Window.h"


Control::Control(IE::control* control)
	:
	fWindow(NULL),
	fControl(control),
	fRoom(NULL)
{
}


Control::~Control()
{
	delete fControl;
}


uint32
Control::ID() const
{
	return fControl->id;
}


/* virtual */
void
Control::Draw()
{
	if (fRoom != NULL)
		fRoom->Draw(NULL);
}


/* virtual */
void
Control::AttachedToWindow(Window* window)
{
	fWindow = window;

	//if (Width() == 640 && Position().x == 0)
		//SetFrame(0, 0, window->Width(), window->Height());
}


/* virtual */
void
Control::MouseMoved(IE::point point, uint32 transit)
{
	if (fRoom != NULL) {
		ConvertFromScreen(point);
		if (point.x >= 0 && point.y >= 0)
			fRoom->MouseOver(point.x, point.y);
	} else if (transit == MOUSE_ENTER) {
		GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);
	}
}


/* virtual */
void
Control::MouseDown(IE::point point)
{
	if (fRoom != NULL) {
		ConvertFromScreen(point);
		if (point.x >= 0 && point.y >= 0)
			fRoom->Clicked(point.x, point.y);
	}
}


/* virtual */
void
Control::MouseUp(IE::point point)
{
}


void
Control::Invoke()
{
	GUI::Get()->ControlInvoked(ID(), fWindow->ID());
}


IE::point
Control::Position() const
{
	IE::point point = { int16(fControl->x), int16(fControl->y) };
	return point;
}


IE::point
Control::ScreenPosition() const
{
	IE::point point = Position();
	fWindow->ConvertToScreen(point);
	return point;
}


uint16
Control::Width() const
{
	return fControl->w;
}


uint16
Control::Height() const
{
	return fControl->h;
}


void
Control::SetFrame(uint16 x, uint16 y, uint16 width, uint16 height)
{
	fControl->x = x;
	fControl->y = y;
	fControl->w = width;
	fControl->h = height;
}


void
Control::ConvertFromScreen(IE::point& point) const
{
	point.x -= Position().x ;
	point.y -= Position().y ;
}


void
Control::AssociateRoom(RoomContainer* room)
{
	fRoom = room;

	GFX::rect viewPortRect(
		Position().x + fWindow->Position().x,
		Position().y + fWindow->Position().y,
		Width(),
		Height()
	);

	if (room != NULL)
		room->SetViewPort(viewPortRect);
}


void
Control::Print() const
{
	fControl->Print();
}


/* static */
Control*
Control::CreateControl(IE::control* control)
{
	switch (control->type) {
		case IE::CONTROL_BUTTON:
			return new Button((IE::button*)control);
		case IE::CONTROL_LABEL:
			return new Label((IE::label*)control);
		case IE::CONTROL_TEXTAREA:
			return new TextArea((IE::text_area*)control);
		case IE::CONTROL_SLIDER:
			return new Slider((IE::slider*)control);
		case IE::CONTROL_SCROLLBAR:
			return new Scrollbar((IE::scrollbar*)control);
		case IE::CONTROL_TEXTEDIT:
			return new TextEdit((IE::text_edit*)control);
		default:
			return new Control(control);
	}
}


/* static */
/*Control*
Control::GetByID(uint32 id)
{
	return sControlsMap[id];
}*/
