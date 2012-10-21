/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "CHUIResource.h"
#include "GUI.h"
#include "ResManager.h"


GUI::GUI(const res_ref& name)
{
	// TODO: There are some cases where there are overlapping windows.
	// How to handle those cases ?
	fResource = gResManager->GetCHUI(name);
	fWindows.push_back(fResource->GetWindow(0));
	/*for (uint32 i = 0; i < fResource->CountWindows(); i++) {
		Window* window = fResource->GetWindow(i);
		fWindows.push_back(window);
	}*/
}


GUI::~GUI()
{
	gResManager->ReleaseResource(fResource);
	fWindows.erase(fWindows.begin(), fWindows.end());
}


void
GUI::Draw()
{
	std::vector<Window*>::const_iterator i;
	for (i = fWindows.begin(); i < fWindows.end(); i++) {
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


Window*
GUI::_GetWindow(IE::point pt)
{
	std::vector<Window*>::const_reverse_iterator i;
	for (i = fWindows.rbegin(); i < fWindows.rend(); i++) {
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
