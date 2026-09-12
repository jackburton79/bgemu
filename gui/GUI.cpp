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
#include "Game.h"
#include "Log.h"
#include "GraphicsEngine.h"
#include "Object.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "TextArea.h"
#include "TextSupport.h"
#include "Timer.h"


#include <algorithm>
#include <cctype>

static GUI* sGUI = NULL;


// Only GUIW files are resolution matched
static bool
IsResolutionMatchedGUIW(const std::string& name)
{
	if (name.size() < 4)
		return false;

	std::string prefix = name.substr(0, 4);
	std::transform(prefix.begin(), prefix.end(), prefix.begin(),
		[](unsigned char c) { return std::toupper(c); });
	if (prefix != "GUIW")
		return false;

	return std::all_of(name.begin() + 4, name.end(),
		[](unsigned char c) { return std::isdigit(c) != 0; });
}


// GUI::ControlInvoked() dispatch tables. Screens Game owns
// (GUISAVE/GUILOAD/GUIINV) are delegated straight to it; these two are
// the handful of controls GUI itself still wires directly.

// World-map scroll-arrow buttons (GUIWMAP window 0): each nudges the map
// view by a fixed pixel delta.
static const struct { uint32 controlID; int16 dx; int16 dy; }
kWorldMapScrollArrows[] = {
	{  1,   0, -20 }, {  2,   0,  20 },
	{  8, -20, -20 }, {  9,  20, -20 },
	{ 10, -20,   0 }, { 12,  20,   0 },
	{ 13, -20,  20 }, { 14,  20,  20 },
};

// HUD command-bar buttons (GUIW* window WINDOW_COMMANDS) - ids confirmed
// by the user while playing (see the Fase 21 plan notes).
static const struct { uint32 controlID; void (*action)(); }
kHUDCommandButtons[] = {
	{  1, [] { Core::Get()->LoadWorldMap(); } },
	{  3, [] { Game::Get()->ToggleInventoryWindow(); } },
	{  4, [] { Game::Get()->ToggleRecordWindow(); } },
	{  5, [] { Game::Get()->ToggleSpellbookWindow(); } },
	{  7, [] { Game::Get()->ToggleSaveWindow(); } },
	{  9, [] { Core::Get()->TogglePause(); } },
	{ 11, [] { Game::Get()->TriggerRest(); } },
};


static void
_LogUnhandledControl(uint16 windowID, uint32 controlID)
{
	std::cout << "GUI: unhandled control " << std::dec << controlID
		<< " in window " << windowID << std::endl;
}


void
DeleteStringEntry(void *param)
{
	long id = reinterpret_cast<long>(param);
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
	fShown(false),
	fTooltipBitmap(NULL),
	fDragBitmap(NULL),
	fHoverTooltipBitmap(NULL),
	fCaptureWindow(NULL)
{
	fCursorPosition.x = 0;
	fCursorPosition.y = 0;

	for (int c = 0; c < NUM_CURSORS; c++)
		fCursors[c] = NULL;
}


GUI::~GUI()
{
	gResManager->ReleaseResource(fResource);

	for (auto tooltip : fTooltipList) {
		if (tooltip.bitmap != nullptr)
			tooltip.bitmap->Release();
	}
	fTooltipList.clear();

	for (auto window: fWindows) {
		delete window;
	}

	fWindows.clear();
	fAuxWindows.clear();
	fAuxWindowSet.clear();

	for (auto& resource : fAuxResources)
		gResManager->ReleaseResource(resource.second);
	fAuxResources.clear();

	fCurrentCursor = NULL;

	if (fDragBitmap != NULL)
		fDragBitmap->Release();
	if (fHoverTooltipBitmap != NULL)
		fHoverTooltipBitmap->Release();

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
	FontRoster::Destroy();
}


bool
GUI::Load(const res_ref& name)
{
	res_ref guiResource = name;
	GFX::rect screenFrame = GraphicsEngine::Get()->ScreenFrame();
	if (::strcasecmp(guiResource.CString(), "GUIW") == 0) {
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
	if (!gResManager->ResourceExists(guiResource, RES_CHU)) {
		// fallback to "normal" gui (BG)
		guiResource = "GUIW";
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


bool
GUI::IsHidden() const
{
	return !fShown;
}


void
GUI::Show()
{
	if (!fShown) {
		fShown = true;
		ShowHUD();
	}
}


void
GUI::Hide()
{
	if (fShown) {
		fShown = false;
		HideHUD();
	}
}


void
GUI::Toggle()
{
	if (IsHidden())
		Show();
	else
		Hide();
}


bool
GUI::IsHUDHidden() const
{
	return !IsWindowShown(WINDOW_COMMANDS);
}


void
GUI::HideHUD()
{
	HideWindow(GUI::WINDOW_MESSAGES);
	HideWindow(GUI::WINDOW_MESSAGES_LARGE);
	HideWindow(GUI::WINDOW_COMMANDS);
	HideWindow(GUI::WINDOW_CMDS);
	HideWindow(GUI::WINDOW_PLAYER_SLOTS);
}


void
GUI::ShowHUD()
{
	// TODO: remember which of WINDOW_MESSAGES/WINDOW_MESSAGES_LARGE was
	// showing before HideHUD() (ToggleMessageArea() switches between the
	// two) instead of always restoring the small one.
	ShowWindow(GUI::WINDOW_MESSAGES);
	ShowWindow(GUI::WINDOW_COMMANDS);
	ShowWindow(GUI::WINDOW_CMDS);
	ShowWindow(GUI::WINDOW_PLAYER_SLOTS);
}


void
GUI::ToggleHUD()
{
	if (IsHUDHidden())
		ShowHUD();
	else
		HideHUD();
}


void
GUI::Draw()
{
	// TODO: is there a better place ?
	UpdateCursorAndScrolling(fCursorPosition.x, fCursorPosition.y);

	for (auto window: fWindows) {
		if (window->Shown()) {
			window->Pulse();
			window->Draw();
		}
	}

	_DrawStrings();

	if (Core::Get()->IsPaused()) {
		const Font* font = FontRoster::GetFont("TOOLFONT");
		std::string text = "Game paused";
		Bitmap* bitmap = font->GetRenderedString(text, 0);
		GFX::rect rect = bitmap->Frame();
		rect.CenterIn(GraphicsEngine::Get()->ScreenFrame());
		GraphicsEngine::Get()->BlitToScreen(bitmap, rect.LeftTop());
		bitmap->Release();
	}
	// If GUI is hidden, don't show cursors
	if (!fShown)
		return;

	if (fDragBitmap != NULL) {
		GFX::rect rect(fCursorPosition.x - fDragBitmap->Width() / 2,
			fCursorPosition.y - fDragBitmap->Height() / 2,
			fDragBitmap->Width(), fDragBitmap->Height());
		GraphicsEngine::Get()->BlitToScreen(fDragBitmap, NULL, &rect);
	}

	if (fHoverTooltipBitmap != NULL && fDragBitmap == NULL) {
		GFX::rect rect(fCursorPosition.x + 12, fCursorPosition.y + 8,
			fHoverTooltipBitmap->Width(), fHoverTooltipBitmap->Height());
		GraphicsEngine::Get()->BlitToScreen(fHoverTooltipBitmap, NULL, &rect);
	}

	if (fCurrentCursor != NULL) {
		try {
			const Bitmap* nextFrame = fCurrentCursor->Bitmap();
			GFX::rect rect(fCursorPosition.x, fCursorPosition.y, 0, 0);
			GraphicsEngine::Get()->BlitToScreen(nextFrame, NULL, &rect);
		} catch (std::exception& e) {
			std::cerr << "GUI::Draw(): " << e.what() << std::endl;
		}
	}
}


void
GUI::SetDragBitmap(Bitmap* bitmap)
{
	if (bitmap == fDragBitmap)
		return;
	if (fDragBitmap != NULL)
		fDragBitmap->Release();
	fDragBitmap = bitmap;
}


bool
GUI::IsDraggingItem() const
{
	return fDragBitmap != NULL;
}


void
GUI::SetHoverTooltip(const std::string& text)
{
	if (text == fHoverTooltipText)
		return;
	fHoverTooltipText = text;
	if (fHoverTooltipBitmap != NULL) {
		fHoverTooltipBitmap->Release();
		fHoverTooltipBitmap = NULL;
	}
	if (text.empty())
		return;
	const Font* font = FontRoster::GetFont("TOOLFONT");
	if (font != NULL)
		fHoverTooltipBitmap = font->GetRenderedString(text, 0);
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
GUI::DisplayMessage(Object* object, const std::string& text)
{
	// Floating text over the object's own position.
	if (object != NULL) {
		GFX::rect frame = rect_to_gfx_rect(object->Frame());
		Core::Get()->CurrentRoom()->ConvertFromArea(frame);
		DisplayStringCentered(text, frame.x, frame.y, 5000);
	}

	// Write text in the message log TextArea too.
	TextArea* textArea = GetMessagesTextArea();
	if (textArea != NULL) {
		std::string fullText;
		if (object != NULL)
			fullText.append(object->Name()).append(": ");
		fullText.append(text);
		textArea->AddText(fullText.c_str());
	}
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
	// Don't touch `window` afterwards: dispatching the click can tear the
	// whole GUI down (e.g. a world-map travel click reloads the area,
	// which calls GUI::Clear()). A control that wants the mouse captured
	// tells GUI directly during its MouseDown (Window::SetMouseCapture ->
	// SetCaptureWindow), and Clear() resets fCaptureWindow.
}


void
GUI::MouseUp(int16 x, int16 y)
{
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	Window* window = fCaptureWindow != NULL ? fCaptureWindow : _WindowAtPoint(point);
	fCaptureWindow = NULL;
	if (window != NULL)
		window->MouseUp(point);
}


void
GUI::SetCaptureWindow(Window* window)
{
	fCaptureWindow = window;
}


void
GUI::RightMouseDown(int16 x, int16 y)
{
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	Window* window = _WindowAtPoint(point);
	if (window != NULL && window->RightMouseDown(point))
		return;

	// Nothing wanted the right click as such - treat it like a normal
	// click so the game world's right-click-to-move keeps working.
	MouseDown(x, y);
}


void
GUI::MouseMoved(int16 x, int16 y)
{
	if (Core::Get()->CutsceneMode())
		return;

	IE::point point = { x, y };
	fCursorPosition = point;

	Window* window = fCaptureWindow != NULL ? fCaptureWindow : _WindowAtPoint(point);
	if (window != NULL)
		window->MouseMoved(point);
}


void
GUI::GetCursorPosition(int16& x, int16& y) const
{
	x = fCursorPosition.x;
	y = fCursorPosition.y;
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
		if (window != NULL) {
			_CenterWindow(window, fResource->Name());
			fWindows.push_back(window);
		}
	}

	// Sort windows based on id
	std::sort(fWindows.begin(), fWindows.end(), WindowsSorter());

	if (window != NULL) {
		window->Show();
		if (window->Frame().Contains(fCursorPosition.x, fCursorPosition.y))
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


bool
GUI::LoadAuxiliary(const res_ref& chuName)
{
	if (fAuxResources.find(chuName) != fAuxResources.end())
		return true; // already loaded

	CHUIResource* resource = gResManager->GetCHUI(chuName);
	if (resource == NULL)
		return false;

	fAuxResources[chuName] = resource;
	return true;
}


void
GUI::ShowAuxWindow(const res_ref& chuName, uint16 windowId)
{
	auto key = std::make_pair(chuName, windowId);
	auto found = fAuxWindows.find(key);
	Window* window = found != fAuxWindows.end() ? found->second : NULL;

	if (window == NULL) {
		if (!LoadAuxiliary(chuName))
			return;
		window = fAuxResources[chuName]->GetWindow(windowId);
		if (window == NULL) {
			std::cerr << Log::Red << "GUI::ShowAuxWindow(): window " << windowId
					<< " not found in " << chuName.CString() << Log::Normal << std::endl;
			return;
		}
		_CenterWindow(window, std::string(chuName.CString()));
		fAuxWindows[key] = window;
		fAuxWindowSet.insert(window);
		AddWindow(window);
	}

	window->Show();
	if (window->Frame().Contains(fCursorPosition.x, fCursorPosition.y))
		window->MouseMoved(fCursorPosition);
}


void
GUI::HideAuxWindow(const res_ref& chuName, uint16 windowId)
{
	auto found = fAuxWindows.find(std::make_pair(chuName, windowId));
	if (found != fAuxWindows.end())
		found->second->Hide();
}


bool
GUI::IsAuxWindowShown(const res_ref& chuName, uint16 windowId) const
{
	auto found = fAuxWindows.find(std::make_pair(chuName, windowId));
	return found != fAuxWindows.end() && found->second->Shown();
}


Window*
GUI::GetAuxWindow(const res_ref& chuName, uint16 windowId) const
{
	auto found = fAuxWindows.find(std::make_pair(chuName, windowId));
	return found != fAuxWindows.end() ? found->second : NULL;
}


void
GUI::ToggleAuxWindow(const res_ref& chuName, uint16 windowId)
{
	if (IsAuxWindowShown(chuName, windowId))
		HideAuxWindow(chuName, windowId);
	else
		ShowAuxWindow(chuName, windowId);
}


bool
GUI::ToggleAuxWindowGroup(const res_ref& chuName,
	std::initializer_list<uint16> windowIds)
{
	bool shown = IsAuxWindowShown(chuName, *windowIds.begin());
	for (uint16 windowId : windowIds)
		HideAuxWindow(chuName, windowId);
	if (!shown) {
		for (uint16 windowId : windowIds)
			ShowAuxWindow(chuName, windowId);
	}
	return !shown;
}


void
GUI::Clear()
{
	for (auto window: fWindows)
		delete window;
	fWindows.clear();
	// The Window objects these pointed to were just deleted above (they
	// were also in fWindows - see ShowAuxWindow()).
	fAuxWindows.clear();
	fAuxWindowSet.clear();

	for (auto& resource : fAuxResources)
		gResManager->ReleaseResource(resource.second);
	fAuxResources.clear();

	// Any in-progress inventory drag belonged to a window just destroyed.
	SetDragBitmap(NULL);
	SetHoverTooltip("");
	fCaptureWindow = NULL;

	_AddBackgroundWindow();
}


Window*
GUI::GetWindow(uint16 id) const
{
	std::vector<Window*>::const_iterator i;
	for (const auto window: fWindows) {
		if (window->ID() == id && fAuxWindowSet.find(window) == fAuxWindowSet.end())
			return window;
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
GUI::_SwitchMessageArea(uint16 fromID, uint16 toID)
{
	TextArea::TextLines lines;
	TextArea* currentTextArea = GetMessagesTextArea();
	if (currentTextArea != NULL)
		currentTextArea->GetLines(lines);

	if (IsWindowShown(fromID))
		HideWindow(fromID);
	ShowWindow(toID);

	TextArea* newTextArea = GetMessagesTextArea();
	if (newTextArea != NULL)
		newTextArea->SetLines(lines);
}


void
GUI::EnsureShowDialogArea()
{
	// A dialog needs the whole HUD back (portraits to pick who's talking,
	// the response options in the message area itself), regardless of
	// whether the player had it hidden via ToggleHUD() - real IE dialogs
	// always show the interface. TerminateDialog() intentionally doesn't
	// mirror this: ending a dialog leaves the HUD as the player last set
	// it, it doesn't re-hide it.
	ShowHUD();
	_SwitchMessageArea(WINDOW_MESSAGES, WINDOW_MESSAGES_LARGE);
}


void
GUI::EnsureShowNormalMessageArea()
{
	_SwitchMessageArea(WINDOW_MESSAGES_LARGE, WINDOW_MESSAGES);
}


void
GUI::ToggleMessageArea()
{
	if (IsWindowShown(WINDOW_MESSAGES))
		EnsureShowDialogArea();
	else if (IsWindowShown(WINDOW_MESSAGES_LARGE))
		EnsureShowNormalMessageArea();
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
	int leftBorder = 0;
	int upperBorder = 0;
	int upperBorderLimit = 0;

	GFX::rect viewPort = room->Control::Frame();
	int rightBorder = viewPort.w;
	int bottomBorder = viewPort.h;

	Control* control = room;
	if (strcmp(room->Name(), "WORLDMAP") == 0) {
		// control->Position() is window-relative; convert to absolute
		// screen coordinates to compare against x/y below (screen-space
		// mouse coordinates) - this used to be a no-op since GUIWMAP's
		// window always sat at (0,0), but isn't once GUI::_CenterWindow()
		// can offset it on larger screens.
		IE::point screenPosition = control->ScreenPosition();
		upperBorder = screenPosition.y + 10;
		upperBorderLimit = screenPosition.y;
		bottomBorder = control->Height() + screenPosition.y - 10;
		leftBorder = screenPosition.x + 10;
		rightBorder = screenPosition.x + viewPort.w - 10;
	} else {
		leftBorder += 15;
		rightBorder -= 15;
		upperBorder += 15;
		bottomBorder -= 15;
	}

	sint16 scrollByX = 0;
	sint16 scrollByY = 0;
	// TODO: handle this better

	if (x <= leftBorder)
		scrollByX = -kScrollingStep;
	else if (x >= rightBorder)
		scrollByX = kScrollingStep;
	if (y <= upperBorder && y >= upperBorderLimit)
		scrollByY = -kScrollingStep;
	else if (y >= bottomBorder)
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
GUI::ControlRightClicked(uint32 controlID, uint16 windowID, const res_ref& chuName)
{
	if (chuName == res_ref("GUIINV"))
		Game::Get()->InventoryControlRightClicked(controlID, windowID);
	else if (chuName == res_ref("GUIMG") || chuName == res_ref("GUIPR"))
		Game::Get()->SpellbookControlRightClicked(controlID, windowID);
}


void
GUI::ControlHovered(uint32 controlID, uint16 windowID, const res_ref& chuName,
					bool inside)
{
	if (chuName == res_ref("GUIINV"))
		Game::Get()->InventoryControlHovered(controlID, windowID, inside);
	else if (chuName == res_ref("GUIMG") || chuName == res_ref("GUIPR"))
		Game::Get()->SpellbookControlHovered(controlID, inside);
}


void
GUI::WindowBackgroundClicked(const res_ref& chuName, uint16 /*windowID*/)
{
	// A click that landed on a window but not on any of its controls.
	// Currently only used to drop a held inventory item onto the floor.
	// Handlers here MUST NOT tear down windows (this runs mid-dispatch,
	// same constraint as ControlInvoked()).
	if (chuName == res_ref("GUIINV"))
		Game::Get()->DropHeldItemOnGround();
}


void
GUI::ControlInvoked(uint32 controlID, uint16 windowID, const res_ref& chuName)
{
	if (chuName == res_ref("GUISAVE") || chuName == res_ref("GUILOAD")) {
		Game::Get()->SaveOrLoadControlInvoked(chuName, controlID, windowID);
		return;
	}

	if (chuName == res_ref("GUIINV")) {
		Game::Get()->InventoryControlInvoked(controlID, windowID);
		return;
	}

	if (chuName == res_ref("GUIREC")) {
		Game::Get()->RecordControlInvoked(controlID, windowID);
		return;
	}

	if (chuName == res_ref("GUIMG") || chuName == res_ref("GUIPR")) {
		Game::Get()->SpellbookControlInvoked(controlID, windowID);
		return;
	}

	if (chuName == res_ref("GUIJRNL")) {
		Game::Get()->JournalControlInvoked(controlID, windowID);
		return;
	}

	RoomBase* room = Core::Get()->CurrentRoom();
	if (room == NULL)
		return;

	if (chuName == res_ref("GUIWMAP")) {
		if (windowID == 0) {
			for (const auto& arrow : kWorldMapScrollArrows) {
				if (arrow.controlID == controlID) {
					room->SetRelativeAreaOffset(arrow.dx, arrow.dy);
					return;
				}
			}
		}
		_LogUnhandledControl(windowID, controlID);
		return;
	}

	if (IsResolutionMatchedGUIW(chuName.CString())) {
		if (windowID == WINDOW_COMMANDS) {
			for (const auto& button : kHUDCommandButtons) {
				if (button.controlID == controlID) {
					button.action();
					return;
				}
			}
		} else if (windowID == WINDOW_PLAYER_SLOTS && controlID <= 5) {
			// The 6 HUD portrait buttons select that party member.
			Game::Get()->SelectPartyMember((uint16)controlID);
			return;
		} else if ((windowID == WINDOW_MESSAGES && controlID == 2)
				|| (windowID == WINDOW_MESSAGES_LARGE && controlID == 0)) {
			ToggleMessageArea();
			return;
		} else if (windowID == WINDOW_MESSAGES && (controlID == 0 || controlID == 1)) {
			// BG1's small message window scrolls via two dedicated arrow
			// buttons instead of a real Scrollbar control (unlike BG2,
			// and unlike BG1's own WINDOW_MESSAGES_LARGE) - same step
			// size as a Scrollbar arrow click (Scrollbar.cpp's own
			// kArrowStep).
			TextArea* textArea = GetMessagesTextArea();
			if (textArea != NULL)
				textArea->ScrollBy(0, controlID == 0 ? -16 : 16);
			return;
		}
		_LogUnhandledControl(windowID, controlID);
		return;
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
		if (window->Shown() && window->Frame().Contains(pt.x, pt.y))
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
GUI::_CenterWindow(Window* window, const std::string& chuName) const
{
	if (window == NULL || IsResolutionMatchedGUIW(chuName))
		return;

	const int16 kBaseWidth = 640;
	const int16 kBaseHeight = 480;
	const int16 offsetX = ((int16)fScreenWidth - kBaseWidth) / 2;
	const int16 offsetY = ((int16)fScreenHeight - kBaseHeight) / 2;
	if (offsetX == 0 && offsetY == 0)
		return;

	IE::point position = window->Position();
	window->MoveTo(position.x + offsetX, position.y + offsetY);
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
	for (const auto& entry: fTooltipList) {
		GraphicsEngine::Get()->BlitToScreen(entry.bitmap, NULL, (GFX::rect*)&entry.rect);
	}
}


// Widest a single line is allowed to get before wrapping - a region's
// info text or a spell's "Text: Display String" effect can be a full
// sentence (or several), and rendering that as one unbroken line used
// to run straight off the edge of the screen.
const static uint16 kMaxMessageLineWidth = 300;


void
GUI::_DisplayStringCommon(const std::string& text,
			uint16 x, uint16 y, bool centerString, uint32 time)
{
	static uint32 sCurrentId = 0;

	const Font* font = FontRoster::GetFont("TOOLFONT");
	// TODO: GetRenderedString always use "true" for palette, while
	// previous call used "false" here. Check!

	// Word-wrap into as many lines as needed to stay under
	// kMaxMessageLineWidth - same TruncateString() line-breaking
	// TextArea::_AddText() already uses for dialog text - then stack
	// each line's own rendered bitmap (GetRenderedString() already
	// produces one sized/paletted/colorkeyed correctly for a single
	// line, same call this used to make just once for the whole text)
	// onto one combined bitmap, tallest-line-first so the combined
	// bitmap's own colorkey/palette come from real rendered text
	// rather than being guessed at.
	std::vector<Bitmap*> lineBitmaps;
	std::string remaining = text;
	uint16 combinedWidth = 0;
	do {
		std::string line = font->TruncateString(remaining, kMaxMessageLineWidth);
		lineBitmaps.push_back(font->GetRenderedString(line, 0));
		combinedWidth = std::max(combinedWidth, lineBitmaps.back()->Width());
	} while (!remaining.empty());

	Bitmap* bitmap = lineBitmaps[0];
	if (lineBitmaps.size() > 1) {
		uint16 combinedHeight = 0;
		for (Bitmap* lineBitmap : lineBitmaps)
			combinedHeight += lineBitmap->Height();

		bitmap = new Bitmap(combinedWidth, combinedHeight, 8);
		bitmap->SetPalette(*GFX::kPaletteYellow);
		uint32 colorKey = 0;
		if (lineBitmaps[0]->GetColorKey(colorKey)) {
			bitmap->SetColorKey(colorKey);
			bitmap->Clear(colorKey);
		}

		GFX::point where(0, 0);
		for (Bitmap* lineBitmap : lineBitmaps) {
			lineBitmap->BlitTo(bitmap, where);
			where.y += lineBitmap->Height();
			lineBitmap->Release();
		}
	}

	// Set the position where to  blit the bitmap
	GFX::rect rect;
	rect.x = x;
	rect.y = y;
	if (centerString) {
		rect.x -= bitmap->Width() / 2;
	}
	string_entry entry = { text, bitmap, rect, sCurrentId};
	fTooltipList.push_back(entry);

	long id = sCurrentId++;
	Timer::AddOneShotTimer(time, DeleteStringEntry, (void*)id);
}
