/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "Animation.h"
#include "BackWindow.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "CHUIResource.h"
#include "Control.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ResManager.h"
#include "Room.h"
#include "TextSupport.h"
#include "Timer.h"

#include <algorithm>

static GUI* sGUI = NULL;


uint32
RemoveString(uint32 interval, void *param)
{
	long id = (long)param;
	sGUI->RemoveToolTip((uint32)id);
	return 0;
}


GUI::GUI(uint16 width, uint16 height)
	:
	fResource(NULL),
	fCurrentCursor(NULL),
	fScreenWidth(width),
	fScreenHeight(height)
{
	for (int c = 0; c < NUM_CURSORS; c++)
		fCursors[c] = NULL;

	fToolTipFontResource = gResManager->GetBAM("TOOLFONT");
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

	gResManager->ReleaseResource(fToolTipFontResource);
}


/* static */
bool
GUI::Initialize(const uint16 width, const uint16 height)
{
	std::cout << "GUI::Initialize(" << std::dec << width;
	std::cout << ", " << height << ")" << std::endl;
	std::flush(std::cout);
	try {
		if (sGUI == NULL)
			sGUI = new GUI(width, height);
	} catch (...) {
		sGUI = NULL;
		return false;
	}
	return true;
}


/* static */
void
GUI::Destroy()
{
	std::cout << "GUI::Destroy()" << std::endl;
	delete sGUI;
	sGUI = NULL;
}


bool
GUI::Load(const res_ref& name)
{
	std::cout << "GUI::Load()" << std::endl;
	try {
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
	} catch (...) {
		std::cout << "GUI::Load(): ERROR" << std::cout;
		return false;
	}
	std::cout << "GUI::Load(): OK!" << std::endl;
	return true;
}


void
GUI::Draw()
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i < fActiveWindows.end(); i++) {
		(*i)->Draw();
	}

	_DrawToolTip();

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
GUI::DrawTooltip(std::string text, uint16 x, uint16 y, uint32 time)
{
	static uint32 sCurrentId = 0;
	string_entry entry = { text, x, y , sCurrentId};

	fTooltipList.push_back(entry);

	long id = sCurrentId;
	Timer::AddOneShotTimer((uint32)time, RemoveString, (void*)id);

	sCurrentId++;
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
	if (IsWindowShown(id))
		return;

	Window* window = fResource->GetWindow(id);
	if (window != NULL) {
		fActiveWindows.push_back(window);
		//window->Print();
	}
}


void
GUI::HideWindow(uint16 id)
{
	std::vector<Window*>::iterator i;
	for (i = fActiveWindows.begin(); i != fActiveWindows.end(); i++) {
		if ((*i)->ID() == id) {
			delete *i;
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
GUI::ToggleWindow(uint16 id)
{
	if (IsWindowShown(id))
		HideWindow(id);
	else
		ShowWindow(id);
}


void
GUI::Clear()
{
	for (std::vector<Window*>::const_iterator i = fActiveWindows.begin();
			i != fActiveWindows.end(); i++) {
		delete *i;
	}
	fActiveWindows.clear();

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
GUI::SetArrowCursor(uint32 index)
{
	fCurrentCursor = fCursors[index];
}


void
GUI::SetCursor(uint32 index)
{
	fCurrentCursor = fCursors[index + 8];
}


void
GUI::ControlInvoked(uint32 controlID, uint16 windowID)
{
	RoomContainer* room = RoomContainer::Get();
	if (room == NULL)
		return;

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


void
GUI::RemoveToolTip(uint32 id)
{
	std::list<string_entry>::iterator i;
	for (i = fTooltipList.begin(); i != fTooltipList.end(); i++) {
		if ((*i).id == id) {
			fTooltipList.erase(i);
			break;
		}
	}
}


/* static */
GUI*
GUI::Get()
{
	return sGUI;
}


Window*
GUI::_GetWindow(IE::point pt)
{
	std::vector<Window*>::reverse_iterator i;
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
	BackWindow* backWindow = new BackWindow(fScreenWidth, fScreenHeight);
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


void
GUI::_DrawToolTip()
{
	std::list<string_entry>::const_iterator i;
	for (i = fTooltipList.begin(); i != fTooltipList.end(); i++) {
		const string_entry& entry = *i;
		GFX::rect rect;
		rect.x = entry.x;
		rect.y = entry.y;
		rect.w = 100;
		rect.h = 30;

		Bitmap* bitmap = GraphicsEngine::Get()->ScreenBitmap();
		TextSupport::RenderString(entry.text,
									fToolTipFontResource,
									0, bitmap, rect);
	}
}
