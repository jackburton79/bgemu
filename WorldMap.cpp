#include "WorldMap.h"

#include "BmpResource.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "Door.h"
#include "Game.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Label.h"
#include "MOSResource.h"
#include "Party.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "TextArea.h"
#include "TisResource.h"
#include "Timer.h"
#include "TLKResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include <SDL.h>


WorldMap::WorldMap()
	:
	fWorldMap(NULL),
	fWorldMapBackground(NULL),
	fWorldMapBitmap(NULL)
{
	GUI* gui = GUI::Get();

	gui->Clear();
	
	if (!gui->Load("GUIWMAP")) {
		throw "Cannot load GUIWMAP";
	}

	gui->ShowWindow(0);

	Core::Get()->EnteredArea(this, NULL);

	SetName("WORLDMAP");

	GraphicsEngine::Get()->SetWindowCaption(Name());

	fWorldMap = gResManager->GetWMAP(Name());

	worldmap_entry entry = fWorldMap->WorldMapEntry();
	fWorldMapBackground = gResManager->GetMOS(entry.background_mos);
	if (fWorldMapBackground == NULL) {
		gResManager->ReleaseResource(fWorldMap);
		throw "Cannot load World map bitmap";
	}
	fWorldMapBitmap = fWorldMapBackground->Image();
	for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
		AreaEntry& areaEntry = fWorldMap->AreaEntryAt(i);
		const Bitmap* iconFrame = areaEntry.Icon();
		IE::point position = areaEntry.Position();
		GFX::rect iconRect(int16(position.x - iconFrame->Frame().w / 2),
					int16(position.y - iconFrame->Frame().h / 2),
					iconFrame->Frame().w, iconFrame->Frame().h);

		GraphicsEngine::Get()->BlitBitmap(iconFrame, NULL,
				fWorldMapBitmap, &iconRect);
	}

	Window* window = gui->GetWindow(0);
	if (window != NULL) {
		// TODO: Move this into GUI ?
		Control* control = window->GetControlByID(4);
		if (control != NULL)
			control->AssociateRoom(this);
	}
}


WorldMap::~WorldMap()
{
	_UnloadWorldMap();
}


/* virtual */
GFX::rect
WorldMap::AreaRect() const
{
	GFX::rect rect;
	rect.x = rect.y = 0;
	rect.w = fWorldMapBitmap->Width();
	rect.h = fWorldMapBitmap->Height();
	return rect;
}


void
WorldMap::Draw(Bitmap *surface)
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (fWorldMap != NULL) {
		//GFX::rect sourceRect = offset_rect(fViewPort,
		//		-fViewPort.x, -fViewPort.y);
		GFX::rect sourceRect = ViewPort();
		sourceRect = offset_rect(sourceRect, AreaOffset().x, AreaOffset().y);
		if (sourceRect.w < gfx->ScreenFrame().w || sourceRect.h < gfx->ScreenFrame().h) {
			GFX::rect clippingRect = ViewPort();
			clippingRect.w = gfx->ScreenFrame().w;
			clippingRect.h = gfx->ScreenFrame().h;
			gfx->SetClipping(&clippingRect);
			gfx->ScreenBitmap()->Clear(0);
			gfx->SetClipping(NULL);
		}
		GFX::rect screenRect = ViewPort();
		gfx->BlitToScreen(fWorldMapBitmap, &sourceRect, &screenRect);
	}
}


void
WorldMap::Clicked(uint16 x, uint16 y)
{
	IE::point point = {int16(x), int16(y)};
	ConvertToArea(point);

	for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
		AreaEntry& area = fWorldMap->AreaEntryAt(i);
		if (rect_contains(area.Rect(), point)) {
			Core::Get()->LoadArea(area.Name(), area.LongName(), NULL);
			break;
		}
	}
}


void
WorldMap::MouseOver(uint16 x, uint16 y)
{
	const uint16 kScrollingStep = 64;

	uint16 horizBorderSize = 35;
	uint32 vertBorderSize = 40;

	// TODO: Less hardcoding of the window number
	Window* window = GUI::Get()->GetWindow(1);
	if (window != NULL) {
		horizBorderSize += window->Width();
	}

	sint16 scrollByX = 0;
	sint16 scrollByY = 0;
	if (x <= horizBorderSize)
		scrollByX = -kScrollingStep;
	else if (x >= ViewPort().w - horizBorderSize)
		scrollByX = kScrollingStep;

	if (y <= vertBorderSize)
		scrollByY = -kScrollingStep;
	else if (y >= ViewPort().h - vertBorderSize)
		scrollByY = kScrollingStep;

	IE::point point = { int16(x), int16(y) };
	ConvertToArea(point);

	_UpdateCursor(x, y, scrollByX, scrollByY);

	if (fWorldMap != NULL) {
		for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
			AreaEntry& area = fWorldMap->AreaEntryAt(i);
			GFX::rect areaRect = area.Rect();
			if (rect_contains(areaRect, point)) {
				ConvertFromArea(areaRect);
				ConvertToScreen(areaRect);
				//char* toolTip = area.TooltipName();
				//RenderString(toolTip, GraphicsEngine::Get()->ScreenSurface());
				//free(toolTip);
				GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(areaRect, 600);
				break;
			}
		}
	}

	SetRelativeAreaOffset(scrollByX, scrollByY);
}


void
WorldMap::ActorEnteredArea(const Actor* actor)
{
	//fActors.push_back(const_cast<Actor*>(actor));
}


void
WorldMap::ActorExitedArea(const Actor* actor)
{
	return;
	/*std::vector<Actor*>::iterator i =
			std::find(fActors.begin(), fActors.end(), actor);
	if (i != fActors.end()) {
		fActors.erase(i);
	}*/
}


void
WorldMap::ToggleGUI()
{
	GUI* gui = GUI::Get();
	if (gui->IsWindowShown(0))
		gui->HideWindow(0);
	else
		gui->ShowWindow(0);

	if (gui->IsWindowShown(1))
		gui->HideWindow(1);
	else
		gui->ShowWindow(1);

	/*if (gui->IsWindowShown(3))
		gui->HideWindow(3);
	else
		gui->ShowWindow(3);*/
}


/* virtual */
void
WorldMap::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
}


void
WorldMap::_UpdateCursor(int x, int y, int scrollByX, int scrollByY)
{
	if (scrollByX == 0 && scrollByY == 0) {
		// TODO: Handle other cursors
		GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);
		return;
	}

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

	GUI::Get()->SetArrowCursor(cursorIndex);
}


void
WorldMap::_UnloadWorldMap()
{
	std::cout << "WorldMap::Unload()" << std::endl;
	assert(fWorldMap != NULL);

	GraphicsEngine::Get()->ScreenBitmap()->Clear(0);

	gResManager->ReleaseResource(fWorldMap);
	fWorldMap = NULL;
	
	gResManager->ReleaseResource(fWorldMapBackground);
	fWorldMapBackground = NULL;
	
	if (fWorldMapBitmap != NULL) {
		fWorldMapBitmap->Release();
		fWorldMapBitmap = NULL;
	}

	//gResManager->TryEmptyResourceCache();
}
