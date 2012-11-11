/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "CHUIResource.h"
#include "GUI.h"
#include "ResManager.h"
#include "Room.h"

GUI gGUI;
static GUI* sGUI = NULL;


GUI::GUI()
	:
	fResource(NULL)
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
	/*else
		Room::CurrentArea()->Clicked(x, y);*/
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
	/*else
		Room::CurrentArea()->MouseOver(x, y);*/
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
	if (window != NULL)
		fActiveWindows.push_back(window);
	window->Print();
	return window;
}



void
GUI::ControlInvoked(uint32 controlID, uint16 windowID)
{
	GameMap* room = GameMap::Get();
	if (!strcmp(fResource->Name(), "GUIWMAP")) {
		switch (windowID) {
			case 0:
				switch (controlID) {
					case 1:
					{
						IE::point point;
						point.x = 0;
						point.y = -20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 2:
					{
						IE::point point;
						point.x = 0;
						point.y = 20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 8:
					{
						IE::point point;
						point.x = -20;
						point.y = -20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 9:
					{
						IE::point point;
						point.x = 20;
						point.y = -20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 10:
					{
						IE::point point;
						point.x = -20;
						point.y = 0;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 12:
					{
						IE::point point;
						point.x = 20;
						point.y = 0;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 13:
					{
						IE::point point;
						point.x = -20;
						point.y = 20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					case 14:
					{
						IE::point point;
						point.x = 20;
						point.y = 20;
						room->SetRelativeAreaOffset(point);
						break;
					}
					default:
						std::cout << "window " << std::dec << windowID << ",";
						std::cout << "control " << controlID << std::endl;
						break;

				}
				break;
			default:
				std::cout << "window " << std::dec << windowID << ",";
				std::cout << "control " << controlID << std::endl;
				break;
			}
	} else if (!strcmp(fResource->Name(), "GUIMAP")) {
		switch (windowID) {
			case 2:
				switch (controlID) {
					case 1:
						room->LoadWorldMap();
						break;
					default:
						std::cout << "window " << std::dec << windowID << ",";
						std::cout << "control " << controlID << std::endl;
						break;
				}
				break;
			default:
				std::cout << "window " << std::dec << windowID << ",";
				std::cout << "control " << controlID << std::endl;
				break;
		}
	}
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
