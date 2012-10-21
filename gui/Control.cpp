/*
 * Control.cpp
 *
 *  Created on: 19/ott/2012
 *      Author: stefano
 */

#include "Button.h"
#include "Control.h"
#include "Label.h"
#include "Scrollbar.h"
#include "Slider.h"
#include "TextArea.h"
#include "Window.h"

std::map<uint32, Control*> Control::sControlsMap;



Control::Control(IE::control* control)
	:
	fWindow(NULL),
	fControl(control)
{
	sControlsMap[control->id] = this;
}


Control::~Control()
{
	sControlsMap[fControl->id] = NULL;
	delete fControl;
}


/* virtual */
void
Control::Draw()
{
}


/* virtual */
void
Control::AttachedToWindow(Window* window)
{
	fWindow = window;
}


/* virtual */
void
Control::MouseMoved(IE::point point, uint32 transit)
{
}


/* virtual */
void
Control::MouseDown(IE::point point)
{
}


/* virtual */
void
Control::MouseUp(IE::point point)
{
}


IE::point
Control::Position() const
{
	IE::point point = { fControl->x, fControl->y };
	return point;
}


IE::point
Control::ScreenPosition() const
{
	IE::point point = { fControl->x, fControl->y };
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
		default:
			return new Control(control);
	}
}


/* static */
Control*
Control::GetByID(uint32 id)
{
	return sControlsMap[id];
}
