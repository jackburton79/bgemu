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
#include "Core.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "TextSupport.h"
#include "Timer.h"

#include "custom/ResourceWindow.h"

#include <algorithm>

static GUI* sGUI = NULL;


uint32
DeleteStringEntry(uint32 interval, void *param)
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
	fScreenHeight(height),
	fShown(true)
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
	fCurrentCursor = NULL;

	for (size_t i = 0; i < NUM_CURSORS; i++) {
		delete fCursors[i];
	}
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
	std::cout << "GUI::Load(" << name.CString() << ")" << std::endl;
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
		std::cout << "GUI::Load(): ERROR" << std::endl;
		return false;
	}
	std::cout << "GUI::Load(): OK!" << std::endl;
	return true;
}


void
GUI::Show()
{
	fShown = true;
	RoomBase* room = Core::Get()->CurrentRoom();
	if (!room->IsGUIShown())
		room->ShowGUI();
}


void
GUI::Hide()
{
	fShown = false;
	RoomBase* room = Core::Get()->CurrentRoom();
	if (room->IsGUIShown())
		room->HideGUI();
}


void
GUI::Draw()
{
	std::vector<Window*>::const_iterator i;
	for (i = fActiveWindows.begin(); i < fActiveWindows.end(); i++) {
		(*i)->Draw();
	}

	_DrawStrings();

	// If GUI is hidden, don't show cursors
	if (!fShown)
		return;

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
GUI::DisplayString(const std::string& text, uint16 x, uint16 y, uint32 time)
{
	_DisplayStringCommon(text, x, y, false, time);
}


void
GUI::DisplayStringCentered(const std::string& text,
			uint16 xCenter, uint16 yCenter, uint32 time)
{
	_DisplayStringCommon(text, xCenter, yCenter, true, time);
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
GUI::GetWindow(uint16 id) const
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
	RoomBase* room = Core::Get()->CurrentRoom();
	if (room == NULL)
		return;

	if (!strcmp(fResource->Name(), "GUIWMAP")) {
		switch (windowID) {
			case 0:
				switch (controlID) {
					case 1:
					{						
						room->SetRelativeAreaOffset(0, -20);
						break;
					}
					case 2:
					{
						room->SetRelativeAreaOffset(0, 20);
						break;
					}
					case 8:
					{
						room->SetRelativeAreaOffset(-20, -20);
						break;
					}
					case 9:
					{
						room->SetRelativeAreaOffset(20, -20);
						break;
					}
					case 10:
					{
						room->SetRelativeAreaOffset(-20, 0);
						break;
					}
					case 12:
					{
						room->SetRelativeAreaOffset(20, 0);
						break;
					}
					case 13:
					{
						room->SetRelativeAreaOffset(-20, 20);
						break;
					}
					case 14:
					{
						room->SetRelativeAreaOffset(20, 20);
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
						Core::Get()->LoadWorldMap();
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
		string_entry &entry = *i;
		if (entry.id == id) {
			entry.bitmap->Release();
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
		if (rect_contains(window->Frame(), pt))
			return window;
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
		fCursors[i] = new Animation("CURSARW", i, false, pt);
	}

	for (int i = 0; i < 40; i++) {
		fCursors[i + 8] = new Animation("CURSORS", i, false, pt);
	}
}


void
GUI::_DrawStrings()
{
	std::list<string_entry>::const_iterator i;
	for (i = fTooltipList.begin(); i != fTooltipList.end(); i++) {
		const string_entry& entry = *i;
		GraphicsEngine::Get()->BlitToScreen(entry.bitmap, NULL, (GFX::rect*)&entry.rect);
	}
	
}


void
GUI::_DisplayStringCommon(const std::string& text,
			uint16 x, uint16 y, bool centerString, uint32 time)
{
	static uint32 sCurrentId = 0;
	
	const Font* font = FontRoster::GetFont("TOOLFONT");
	uint16 height;
	uint16 stringWidth = font->StringWidth(text, &height);
	Bitmap* bitmap = new Bitmap(stringWidth, height, 8);
	bitmap->Clear(font->TransparentIndex());
	//bitmap->SetColorKey(font->TransparentIndex());
	
	// Pre-render the string to a bitmap
	GFX::rect rect(0, 0, bitmap->Width(), bitmap->Height());
	font->RenderString(text, 0, bitmap, rect);
	
	// Set the position where to  blit the bitmap
	rect.x = x;
	rect.y = y;
	if (centerString) {
		rect.x -= stringWidth / 2;
	}
	string_entry entry = { text, bitmap, rect, sCurrentId};
	fTooltipList.push_back(entry);

	long id = sCurrentId++;
	Timer::AddOneShotTimer((uint32)time, DeleteStringEntry, (void*)id);
}
