#include "WorldMap.h"

#include "BmpResource.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Label.h"
#include "MOSResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "TextArea.h"
#include "TisResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdexcept>


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
		throw std::runtime_error("Cannot load GUIWMAP");
	}

	gui->ShowWindow(0);

	SetName("WORLDMAP");

	GraphicsEngine::Get()->SetWindowCaption(Name());

	fWorldMap = gResManager->GetWMAP(Name());

	worldmap_entry entry = fWorldMap->WorldMapEntry();
	fWorldMapBackground = gResManager->GetMOS(entry.background_mos);
	if (fWorldMapBackground == NULL) {
		gResManager->ReleaseResource(fWorldMap);
		throw std::runtime_error("Cannot load World map bitmap");
	}
	fWorldMapBitmap = fWorldMapBackground->Image();
	std::cout << "World map bitmap rect:" << std::endl;
	fWorldMapBitmap->Frame().Print();
	_LoadAreaEntries();

	for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
		AreaEntry* areaEntry = fAreaEntries.at(i);
		const Bitmap* iconFrame = areaEntry->Icon();
		IE::point position = areaEntry->Position();
		GFX::rect iconRect(int16(position.x - iconFrame->Frame().w / 2),
					int16(position.y - iconFrame->Frame().h / 2),
					iconFrame->Frame().w, iconFrame->Frame().h);

		GraphicsEngine::Get()->BlitBitmap(iconFrame, NULL,
				fWorldMapBitmap, &iconRect);
	}

	::Window* window = gui->GetWindow(0);
	if (window != NULL) {
		fSavedControl = window->ReplaceControl(4, this);
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
WorldMap::Draw()
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (fWorldMap != NULL) {
		GFX::rect sourceRect = ViewPort();
		ConvertToArea(sourceRect);
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
	if (fAreaUnderMouse != NULL)
		Core::Get()->LoadArea(fAreaUnderMouse->Name(), fAreaUnderMouse->LongName(), NULL);
}


void
WorldMap::MouseMoved(IE::point point, uint32 transit)
{
	ConvertFromScreen(point);
	ConvertToArea(point);

	fAreaUnderMouse = NULL;

	for (uint32 i = 0; i < fAreaEntries.size(); i++) {
		AreaEntry* area = fAreaEntries.at(i);
		if (rect_contains(area->Rect(), point)) {
			fAreaUnderMouse = area;
			break;
		}
	}

	GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);

	::Window* window = GUI::Get()->GetWindow(0);
	if (window == NULL)
		return;
	Label* label = dynamic_cast<Label*>(window->GetControlByID(268435458));
	if (label != NULL) {
		if (fAreaUnderMouse != NULL) {
			label->SetText(fAreaUnderMouse->Caption());
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
WorldMap::ReloadArea()
{
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

	// TODO: here we could have been called by the Window destructor,
	// so some of these fields could be already have been deleted
	Window()->ReplaceControl(fControlID, fSavedControl);

	GraphicsEngine::Get()->ScreenBitmap()->Clear(0);

	std::vector<AreaEntry*>::const_iterator i;
	for (i = fAreaEntries.begin(); i != fAreaEntries.end(); i++) {
		delete *i;
	}
	fAreaEntries.clear();

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


void
WorldMap::_LoadAreaEntries()
{
	for (uint32 c = 0; c < fWorldMap->CountAreaEntries(); c++) {
		AreaEntry* areaEntry = fWorldMap->GetAreaEntry(c);
		fAreaEntries.push_back(areaEntry);
	}
}
