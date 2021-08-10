/*
 * GUI.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "GUI.h"

#include "Animation.h"
#include "BackWindow.h"
#include "BamResource.h"
#include "Bitmap.h"
#include "CHUIResource.h"
#include "Control.h"
#include "Core.h"
#include "Log.h"
#include "GraphicsEngine.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "TextArea.h"
#include "TextSupport.h"
#include "Timer.h"

#include "custom/ResourceWindow.h"

#include <algorithm>

static GUI* sGUI = NULL;


void
DeleteStringEntry(void *param)
{
	long id = (long)param;
	sGUI->RemoveToolTip((uint32)id);
}


GUI::GUI(uint16 width, uint16 height)
	:
	fResource(NULL),
	fBackWindow(NULL),
	fCurrentCursor(NULL),
	fScreenWidth(width),
	fScreenHeight(height),
	fLastScrollTime(0),
	fShown(true),
	fTooltipBitmap(NULL)
{
	fCursorPosition.x = 0;
	fCursorPosition.y = 0;

	for (int c = 0; c < NUM_CURSORS; c++)
		fCursors[c] = NULL;
}


GUI::~GUI()
{
	gResManager->ReleaseResource(fResource);
	for (std::vector<Window*>::const_iterator iter = fWindows.begin();
			iter != fWindows.end(); iter++) {
		delete *iter;
	}

	fWindows.clear();
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
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
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
	res_ref guiResource = name;
	GFX::rect screenFrame = GraphicsEngine::Get()->ScreenFrame();
	if (strcmp(guiResource.CString(), "GUIW") == 0) {
		switch (screenFrame.w) {
			case 800:
				guiResource = "GUIW08";
				break;
			case 1024:
				guiResource = "GUIW10";
				break;
			default:
				break;
		}
	}
	std::cout << Log::Normal;
	std::cout << "GUI::Load(" << guiResource.CString() << "): ";
	try {
		if (fCursors[0] == NULL)
			_InitCursors();

		gResManager->ReleaseResource(fResource);
		Clear();

		fResource = gResManager->GetCHUI(guiResource);
	} catch (std::exception& e) {
		std::cout << Log::Red << e.what() << std::endl;
		return false;
	}
	std::cout << Log::Green << "OK!" << std::endl;
	std::cout << Log::Normal;
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
	// TODO: is there a better place ?
	UpdateCursorAndScrolling(fCursorPosition.x, fCursorPosition.y);

	std::vector<Window*>::const_iterator i;
	for (i = fWindows.begin(); i < fWindows.end(); i++) {
		Window* window = *i;
		if (window->Shown()) {
			window->Pulse();
			window->Draw();
		}
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
		} catch (std::exception& e) {
			std::cerr << "GUI::Draw(): " << e.what() << std::endl;
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
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	Window* window = _WindowAtPoint(point);
	if (window != NULL)
		window->MouseDown(point);
}


void
GUI::MouseUp(int16 x, int16 y)
{
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	Window* window = _WindowAtPoint(point);
	if (window != NULL)
		window->MouseUp(point);
}


void
GUI::MouseMoved(int16 x, int16 y)
{
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	fCursorPosition = point;

	Window* window = _WindowAtPoint(point);
	if (window != NULL)
		window->MouseMoved(point);
}


struct WindowsSorter {
	bool operator()(Window* const& window1, Window* const& window2) const {
		return window1->ID() > window2->ID();
	}
};


void
GUI::ShowWindow(uint16 id)
{
	Window* window = GetWindow(id);
	if (window == NULL) {
		window = fResource->GetWindow(id);
		if (window != NULL)
			fWindows.push_back(window);
	}

	// Sort windows based on id
	std::sort(fWindows.begin(), fWindows.end(), WindowsSorter());

	if (window != NULL) {
		window->Show();
		if (rect_contains(window->Frame(), fCursorPosition))
			window->MouseMoved(fCursorPosition);
	}
}


void
GUI::AddWindow(Window* window)
{
	fWindows.push_back(window);
}


void
GUI::HideWindow(uint16 id)
{
	Window* window = GetWindow(id);
	if (window != NULL)
		window->Hide();
}


bool
GUI::IsWindowShown(uint16 id) const
{
	Window* window = GetWindow(id);
	if (window != NULL && window->Shown())
		return true;

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
	for (std::vector<Window*>::const_iterator i = fWindows.begin();
			i != fWindows.end(); i++) {
		delete *i;
	}
	fWindows.clear();

	_AddBackgroundWindow();
}


Window*
GUI::GetWindow(uint16 id) const
{
	std::vector<Window*>::const_iterator i;
	for (i = fWindows.begin(); i != fWindows.end(); i++) {
		if ((*i)->ID() == id)
			return *i;
	}

	return NULL;
}


TextArea*
GUI::GetMessagesTextArea()
{
	Window* window = GUI::Get()->GetWindow(GUI::WINDOW_MESSAGES);
	TextArea* textArea = NULL;
	if (window != NULL && window->Shown()) {
		textArea = dynamic_cast<TextArea*>(
									window->GetControlByID(3));
	} else {
		window = GUI::Get()->GetWindow(GUI::WINDOW_MESSAGES_LARGE);
		if (window != NULL && window->Shown()) {
			textArea = dynamic_cast<TextArea*>(
					window->GetControlByID(1));
		}
	}
	return textArea;
}


void
GUI::EnsureShowDialogArea()
{
	if (IsWindowShown(WINDOW_MESSAGES)) {
		HideWindow(WINDOW_MESSAGES);
		ShowWindow(WINDOW_MESSAGES_LARGE);
	}
}


void
GUI::ToggleMessageArea()
{
	TextArea::TextLines lines;
	TextArea* currentTextArea = GetMessagesTextArea();
	if (currentTextArea != NULL)
		currentTextArea->GetLines(lines);
	
	if (IsWindowShown(WINDOW_MESSAGES)) {
		HideWindow(WINDOW_MESSAGES);
		ShowWindow(WINDOW_MESSAGES_LARGE);
	} else if (IsWindowShown(WINDOW_MESSAGES_LARGE)) {
		HideWindow(WINDOW_MESSAGES_LARGE);
		ShowWindow(WINDOW_MESSAGES);
	}
	TextArea* newTextArea = GetMessagesTextArea();
	if (newTextArea != NULL)
		newTextArea->SetLines(lines);
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
GUI::UpdateCursorAndScrolling(int x, int y)
{
	RoomBase* room = Core::Get()->CurrentRoom();
	if (room == NULL)
		return;

	const uint16 kScrollingStep = 64;
	int xMinBorder = 0;
	int yMinBorder = 0;
	
	GFX::rect viewPort = room->ViewPort();
	int xMaxBorder = viewPort.w;
	int yMaxBorder = viewPort.h;

	Control* control = room;
	if (strcmp(room->Name(), "WORLDMAP") == 0) {
		yMinBorder = control->Position().y;
		yMaxBorder = control->Height() + control->Position().y;
		xMinBorder += 10;
		xMaxBorder -= 10;
	} else {
		xMinBorder += 15;
		xMaxBorder -= 15;
		yMinBorder += 15;
		yMaxBorder -= 15;
	}

	sint16 scrollByX = 0;
	sint16 scrollByY = 0;
	if (x <= xMinBorder)
		scrollByX = -kScrollingStep;
	else if (x >= xMaxBorder)
		scrollByX = kScrollingStep;
	if (y <= yMinBorder)
		scrollByY = -kScrollingStep;
	else if (y >= yMaxBorder)
		scrollByY = kScrollingStep;

	if (scrollByX == 0 && scrollByY == 0)
		return;

	int cursorIndex = 0;
	if (scrollByX > 0) {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_SE;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_NE;
		else
			cursorIndex = IE::CURSOR_ARROW_E;
	} else if (scrollByX < 0) {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_SW;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_NW;
		else
			cursorIndex = IE::CURSOR_ARROW_W;
	} else {
		if (scrollByY > 0)
			cursorIndex = IE::CURSOR_ARROW_S;
		else if (scrollByY < 0)
			cursorIndex = IE::CURSOR_ARROW_N;
		else
			cursorIndex = IE::CURSOR_ARROW_E;
	}

	SetArrowCursor(cursorIndex);

	const uint32 kScrollDelay = 100;

	uint32 ticks = Timer::Ticks();
	if (fLastScrollTime + kScrollDelay < ticks) {
		Core::Get()->CurrentRoom()->SetRelativeAreaOffset(scrollByX, scrollByY);
		fLastScrollTime = ticks;
	}
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
	} else if (!strncmp(fResource->Name(), "GUIW", strlen("GUIW"))) {
		switch (windowID) {
			case WINDOW_COMMANDS:
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
			case WINDOW_MESSAGES:
				switch (controlID) {
					case 2:
						ToggleMessageArea();
						break;
					default:
						std::cout << "window " << std::dec << windowID << ",";
						std::cout << "control " << controlID << std::endl;
						break;
				}
				break;
			case WINDOW_MESSAGES_LARGE:
				switch (controlID) {
					case 0:
						ToggleMessageArea();
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
GUI::_WindowAtPoint(IE::point pt)
{
	std::vector<Window*>::reverse_iterator i;
	for (i = fWindows.rbegin(); i < fWindows.rend(); i++) {
		Window* window = (*i);
		if (window->Shown() && rect_contains(window->Frame(), pt))
			return window;
	}

	return NULL;
}


void
GUI::_AddBackgroundWindow()
{
	fBackWindow = new BackWindow(fScreenWidth, fScreenHeight);
	fWindows.push_back(fBackWindow);
}


void
GUI::_InitCursors()
{
	IE::point pt;
	try {
		for (int i = 0; i < 8; i++) {
			fCursors[i] = new Animation("CURSARW", i, false, pt);
		}
	} catch (...) {
		std::cerr << "GUI::_InitCursors(): Failed to load arrow cursors" << std::endl;
	}
	try {
		for (int i = 0; i < 40; i++) {
			fCursors[i + 8] = new Animation("CURSORS", i, false, pt);
		}
	} catch (...) {
		std::cerr << "GUI::_InitCursors(): Failed to load cursors" << std::endl;
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
	//bitmap->Clear(font->TransparentIndex());
	//bitmap->SetColorKey(font->TransparentIndex());
	
	// Pre-render the string to a bitmap
	GFX::rect rect(0, 0, bitmap->Width(), bitmap->Height());
	font->RenderString(text, 0, bitmap, false, rect);
	
	// Set the position where to  blit the bitmap
	rect.x = x;
	rect.y = y;
	if (centerString) {
		rect.x -= stringWidth / 2;
	}
	string_entry entry = { text, bitmap, rect, sCurrentId};
	fTooltipList.push_back(entry);

	long id = sCurrentId++;
	Timer::AddPeriodicTimer(time, DeleteStringEntry, (void*)id);
}
