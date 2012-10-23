/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "CHUIResource.h"
#include "GUI.h"
#include "ResManager.h"

GUI gGUI;
static GUI* sGUI = NULL;

GUI::GUI()
{
	sGUI = &gGUI;
}


GUI::~GUI()
{
	gResManager->ReleaseResource(fResource);
	fActiveWindows.erase(fActiveWindows.begin(), fActiveWindows.end());
}


void
GUI::Load(const res_ref& name)
{
	gResManager->ReleaseResource(fResource);
	Clear();
	fResource = gResManager->GetCHUI(name);
}


void
GUI::Draw()
{
	std::vector<Window*>::const_reverse_iterator i;
	for (i = fActiveWindows.rbegin(); i < fActiveWindows.rend(); i++) {
		(*i)->Draw();
	}
}


void
GUI::MouseDown(int16 x, int16 y)
{
	IE::point point = { x, y };
	Window* window = _GetWindow(point);
	if (window != NULL)
		window->MouseDown(point);
}


void
GUI::MouseUp(int16 x, int16 y)
{
	IE::point point = { x, y };
	Window* window = _GetWindow(point);
	if (window != NULL)
		window->MouseUp(point);
}


void
GUI::MouseMoved(int16 x, int16 y)
{
	IE::point point = { x, y };
	Window* window = _GetWindow(point);
	if (window != NULL)
		window->MouseMoved(point);
}


void
GUI::Clear()
{
	fActiveWindows.erase(fActiveWindows.begin(), fActiveWindows.end());
}


Window*
GUI::GetWindow(uint32 id)
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i < fActiveWindows.end(); i++) {
		if ((*i)->ID() == id)
			return *i;
	}
	Window* window = fResource->GetWindow(id);
	fActiveWindows.push_back(window);
	return window;
}


/* static */
GUI*
GUI::Default()
{
	return sGUI;
}


Window*
GUI::_GetWindow(IE::point pt)
{
	std::vector<Window*>::const_reverse_iterator i;
	for (i = fActiveWindows.rbegin(); i < fActiveWindows.rend(); i++) {
		Window* window = (*i);
		if (pt.x >= window->Position().x
				&& pt.x <= window->Position().x + window->Width()
				&& pt.y >= window->Position().y
				&& pt.y <= window->Position().y + window->Height()) {
			return window;
		}
	}

	return NULL;
}
