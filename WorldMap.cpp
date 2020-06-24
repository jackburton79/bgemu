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
	fWorldMapBitmap(NULL),
	fAreaUnderMouse(NULL)
{
	GUI* gui = GUI::Get();

	gui->Clear();
	
	if (!gui->Load("GUIWMAP")) {
		throw "Cannot load GUIWMAP";
	}

	gui->ShowWindow(0);

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
		ViewPort().Print();
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
		GFX::rect sourceRect = ViewPort();
		ConvertToArea(sourceRect);
		if (sourceRect.w < gfx->ScreenFrame().w || sourceRect.h < gfx->ScreenFrame().h) {
			GFX::rect clippingRect = ViewPort();
			clippingRect.w = gfx->ScreenFrame().w;
			clippingRect.h = gfx->ScreenFrame().h;
			gfx->SetClipping(&clippingRect);
			gfx->ScreenBitmap()->Clear(0);
			gfx->SetClipping(NULL);
		}
		GFX::rect visibleArea = rect_to_gfx_rect(VisibleMapArea());
		GFX::rect viewPort = ViewPort();
		gfx->BlitToScreen(fWorldMapBitmap, &visibleArea, &viewPort);
		if (fAreaUnderMouse != NULL) {
			GFX::rect areaRect = fAreaUnderMouse->Rect();
			areaRect.x += ViewPort().x - AreaOffset().x;
			areaRect.y += ViewPort().y - AreaOffset().y;
			GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(areaRect, 600);
		}
	}
}


void
WorldMap::MouseDown(IE::point point)
{
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
WorldMap::MouseMoved(IE::point point, uint32 transit)
{
	ConvertToArea(point);

	fAreaUnderMouse = NULL;

	if (fWorldMap != NULL) {
		for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
			AreaEntry& area = fWorldMap->AreaEntryAt(i);
			GFX::rect areaRect = area.Rect();
			if (rect_contains(areaRect, point)) {
				fAreaUnderMouse = &area;
				break;
			}
		}
	}

	GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);

	Window* window = GUI::Get()->GetWindow(0);
	if (window == NULL)
		return;
	Label* label = dynamic_cast<Label*>(window->GetControlByID(268435458));
	if (label != NULL) {
		if (fAreaUnderMouse != NULL) {
			label->SetText(fAreaUnderMouse->Caption().c_str());
		} else
			label->SetText("");
	}
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
}


/* virtual */
void
WorldMap::ShowGUI()
{
}


/* virtual */
void
WorldMap::HideGUI()
{
}


/* virtual */
bool
WorldMap::IsGUIShown() const
{
	return true;
}


/* virtual */
void
WorldMap::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
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

	gResManager->TryEmptyResourceCache();
}
