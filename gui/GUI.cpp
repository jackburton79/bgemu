/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "Animation.h"
#include "BackWindow.h"
#include "Bitmap.h"
#include "CHUIResource.h"
#include "Control.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ResManager.h"
#include "Room.h"


#include <algorithm>

static GUI* sGUI = NULL;


GUI::GUI()
	:
	fResource(NULL),
	fCurrentCursor(NULL)
{
	for (int c = 0; c < NUM_CURSORS; c++)
		fCursors[c] = NULL;
}


GUI::~GUI()
{
	gResManager->ReleaseResource(fResource);
	for (std::vector<Window*>::const_iterator iter = fActiveWindows.begin();
			iter != fActiveWindows.end(); iter++) {
		delete *iter;
	}

	fActiveWindows.clear();

	for (size_t i = 0; i < NUM_CURSORS; i++) {
		delete fCursors[i];
	}
}


void
GUI::Load(const res_ref& name)
{
	if (fCursors[0] == NULL)
		_InitCursors();

	gResManager->ReleaseResource(fResource);
	Clear();
	fResource = gResManager->GetCHUI(name);

	/*for (uint16 c = 0; c < fResource->CountWindows(); c++) {
		Window* window = fResource->GetWindow(c);
		if (window != NULL)
			fWindows.push_back(window);
	}*/
}


void
GUI::Draw()
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i < fActiveWindows.end(); i++) {
		(*i)->Draw();
	}

	if (fCurrentCursor != NULL) {
		try {
			const Bitmap* nextFrame = fCurrentCursor->NextBitmap();
			GFX::rect rect(fCursorPosition.x, fCursorPosition.y, 0, 0);
			GraphicsEngine::Get()->BlitToScreen(nextFrame, NULL, &rect);
		} catch (const char* string) {
			std::cerr << "Room::Draw(): " << string << std::endl;
		} catch (...) {
			// ...
		}

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
	fCursorPosition = point;

	Window* window = _GetWindow(point);
	if (window != NULL)
		window->MouseMoved(point);
}


void
GUI::ShowWindow(uint16 id)
{
	Window* window = fResource->GetWindow(id);
	if (window != NULL) {
		fActiveWindows.push_back(window);
		window->Print();
	}
}


void
GUI::HideWindow(uint16 id)
{
	std::vector<Window*>::iterator i;
	for (i = fActiveWindows.begin(); i != fActiveWindows.end(); i++) {
		if ((*i)->ID() == id) {
			fActiveWindows.erase(i);
			break;
		}
	}
}


bool
GUI::IsWindowShown(uint16 id) const
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i != fActiveWindows.end(); i++) {
		if ((*i)->ID() == id)
			return true;
	}
	return false;
}


void
GUI::Clear()
{
	fActiveWindows.clear();
	fActiveWindows.erase(fActiveWindows.begin(), fActiveWindows.end());

	_AddBackgroundWindow();
}


Window*
GUI::GetWindow(uint16 id)
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i != fActiveWindows.end(); i++) {
		if ((*i)->ID() == id)
			return *i;
	}

	return NULL;
}


void
GUI::SetCursor(uint32 index)
{
	fCurrentCursor = fCursors[index];
}


void
GUI::ControlInvoked(uint32 controlID, uint16 windowID)
{
	Room* room = Room::Get();
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
	if (sGUI == NULL)
		sGUI = new GUI();
	return sGUI;
}


/* static */
void
GUI::Destroy()
{
	delete sGUI;
	sGUI = NULL;
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


void
GUI::_AddBackgroundWindow()
{
	BackWindow* backWindow = new BackWindow();
	//fWindows.push_back(backWindow);
	fActiveWindows.push_back(backWindow);
}


void
GUI::_InitCursors()
{
	IE::point pt;
	for (int i = 0; i < 8; i++) {
		fCursors[i] = new Animation("CURSARW", i, pt);
	}

	for (int i = 0; i < 40; i++) {
		fCursors[i + 8] = new Animation("CURSORS", i, pt);
	}

}
