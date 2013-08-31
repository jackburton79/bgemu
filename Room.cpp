#include "Actor.h"
#include "Animation.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "BmpResource.h"
#include "Bitmap.h"
#include "Control.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Label.h"
#include "MOSResource.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "TextArea.h"
#include "TileCell.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <assert.h>
#include <iostream>

std::vector<MapOverlay*> *gOverlays = NULL;

static Room* sCurrentRoom = NULL;

static int sSelectedActorRadius = 20;
static int sSelectedActorRadiusStep = 1;

Room::Room()
	:
	Object(""),
	fName(""),
	fWed(NULL),
	fArea(NULL),
	fBcs(NULL),
	fWorldMap(NULL),
	fWorldMapBackground(NULL),
	fWorldMapBitmap(NULL),
	fBackBitmap(NULL),
	fBlitMask(NULL),
	fHeightMap(NULL),
	fLightMap(NULL),
	fSearchMap(NULL),
	fSelectedActor(NULL),
	fMouseOverObject(NULL),
	fDrawSearchMap(0),
	fDrawOverlays(true),
	fDrawPolygons(false),
	fDrawAnimations(true),
	fShowingConsole(false)
{
	sCurrentRoom = this;
}


Room::~Room()
{
	// TODO: Delete various tilecells, overlays, animations
	_UnloadArea();
	_UnloadWorldMap();
	GraphicsEngine::DeleteBitmap(fBackBitmap);
	GraphicsEngine::DeleteBitmap(fBlitMask);
}


res_ref
Room::AreaName() const
{
	return fName;
}


WEDResource*
Room::WED()
{
	return fWed;
}


GFX::rect
Room::ViewPort() const
{
	return fViewPort;
}


bool
Room::LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName)
{
	// Save the entrance name, it will be unloaded in UnloadArea
	std::string savedEntranceName = entranceName ? entranceName : "";

	_UnloadArea();
	_UnloadWorldMap();

	fName = areaName;

	GraphicsEngine::Get()->SetWindowCaption(fName.CString());

	std::cout << "Room::Load(" << areaName.CString() << ")" << std::endl;

	fArea = gResManager->GetARA(fName);
	if (fArea == NULL)
		return false;

	fWed = gResManager->GetWED(fArea->WedName());
	if (fWed == NULL)
		return false;

	fBcs = gResManager->GetBCS(fArea->ScriptName());
	Script* roomScript = NULL;
	if (fBcs != NULL)
		roomScript = fBcs->GetScript();

	GUI* gui = GUI::Get();
	gui->Clear();
	gui->Load("GUIW");
	gui->ShowWindow(uint16(-1));
	Window* window = gui->GetWindow(uint16(-1));

	gui->ShowWindow(0);
	gui->ShowWindow(1);
	/*gui->ShowWindow(2);
	gui->ShowWindow(4);
	if (Window* tmp = gui->GetWindow(4)) {
		TextArea *textArea = dynamic_cast<TextArea*>(tmp->GetControlByID(3));
		if (textArea != NULL)
			textArea->SetText("This is a test");
	}*/
	//gui->GetWindow(15);


	if (window != NULL) {
		Control* control = window->GetControlByID(uint32(-1));
		if (control != NULL)
			control->AssociateRoom(this);
		Label* label = dynamic_cast<Label*>(window->GetControlByID(268435459));
		if (label != NULL)
			label->SetText(longName);
	}
	_LoadOverlays();
	_InitTileCells();
	_InitVariables();
	_InitAnimations();
	_InitRegions();
	_LoadActors();
	_InitDoors();

	_InitBitmap(fViewPort);

	_InitHeightMap();
	_InitLightMap();
	_InitSearchMap();

	_InitBlitMask();

	Core::Get()->EnteredArea(this, roomScript);
	delete roomScript;

	if (!savedEntranceName.empty()) {
		for (uint32 e = 0; e < fArea->CountEntrances(); e++) {
			IE::entrance entrance = fArea->EntranceAt(e);
			std::cout << "current: " << entrance.name;
			std::cout << ", looking for " << entranceName << std::endl;

			if (savedEntranceName == entrance.name) {
				IE::point point = { entrance.x, entrance.y };
				SetAreaOffset(point);
				break;
			}
		}
	} else {
		IE::point point = { 0, 0 };
		SetAreaOffset(point);
	}
	return true;
}


bool
Room::LoadArea(AreaEntry& area)
{
	//MOSResource* mos = gResManager->GetMOS(area.LoadingScreenName());
	/*MOSResource* mos = gResManager->GetMOS("BACK");
	if (mos != NULL) {
		Bitmap* loadingScreen = mos->Image();
		if (loadingScreen != NULL) {
			GraphicsEngine::Get()->BlitToScreen(loadingScreen, NULL, NULL);
			GraphicsEngine::Get()->Flip();
			SDL_Delay(2000);
			GraphicsEngine::DeleteBitmap(loadingScreen);
		}
		gResManager->ReleaseResource(mos);
	}*/

	bool result = LoadArea(area.Name(), area.LongName());

	return result;
}


bool
Room::LoadWorldMap()
{
	if (fWorldMap != NULL)
		return true;

	_UnloadArea();

	GUI* gui = GUI::Get();

	gui->Clear();
	gui->Load("GUIWMAP");

	gui->ShowWindow(0);
	Window* window = gui->GetWindow(0);
	if (window != NULL) {
		// TODO: Move this into GUI ?
		Control* control = window->GetControlByID(4);
		if (control != NULL)
			control->AssociateRoom(this);
	}

	fAreaOffset.x = fAreaOffset.y = 0;

	Core::Get()->EnteredArea(this, NULL);

	fName = "WORLDMAP";

	GraphicsEngine::Get()->SetWindowCaption(fName.CString());

	fWorldMap = gResManager->GetWMAP(fName);

	worldmap_entry entry = fWorldMap->WorldMapEntry();
	fWorldMapBackground = gResManager->GetMOS(entry.background_mos);
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
	return true;
}


void
Room::SetViewPort(GFX::rect rect)
{
	fViewPort = rect;
}


GFX::rect
Room::AreaRect() const
{
	GFX::rect rect;
	rect.x = rect.y = 0;
	if (fWorldMap != NULL) {
		rect.w = fWorldMapBitmap->Width();
		rect.h = fWorldMapBitmap->Height();
	} else {
		rect.w = fOverlays[0]->Width() * TILE_WIDTH;
		rect.h = fOverlays[0]->Height() * TILE_HEIGHT;
	}
	return rect;
}


IE::point
Room::AreaOffset() const
{
	return fAreaOffset;
}


void
Room::SetAreaOffset(IE::point point)
{
	GFX::rect areaRect = AreaRect();
	fAreaOffset = point;
	if (fAreaOffset.x < 0)
		fAreaOffset.x = 0;
	else if (fAreaOffset.x + fViewPort.w > areaRect.w)
		fAreaOffset.x = std::max(areaRect.w - fViewPort.w, 0);
	if (fAreaOffset.y < 0)
		fAreaOffset.y = 0;
	else if (fAreaOffset.y + fViewPort.h > areaRect.h)
		fAreaOffset.y = std::max(areaRect.h - fViewPort.h, 0);

	fMapArea = offset_rect_to(fViewPort,
			fAreaOffset.x, fAreaOffset.y);
}


void
Room::SetRelativeAreaOffset(IE::point relativePoint)
{
	IE::point newOffset = fAreaOffset;
	newOffset.x += relativePoint.x;
	newOffset.y += relativePoint.y;
	SetAreaOffset(newOffset);
}


void
Room::ConvertToArea(GFX::rect& rect)
{
	rect.x += fAreaOffset.x;
	rect.y += fAreaOffset.y;
}


void
Room::ConvertToArea(IE::point& point)
{
	point.x += fAreaOffset.x;
	point.y += fAreaOffset.y;
}


void
Room::ConvertFromArea(GFX::rect& rect)
{
	rect.x -= fAreaOffset.x;
	rect.y -= fAreaOffset.y;
}


void
Room::ConvertFromArea(IE::point& point)
{
	point.x -= fAreaOffset.x;
	point.y -= fAreaOffset.y;
}


void
Room::ConvertToScreen(GFX::rect& rect)
{
	rect.x += fViewPort.x;
	rect.y += fViewPort.y;
}


void
Room::ConvertToScreen(IE::point& point)
{
	point.x += fViewPort.x;
	point.y += fViewPort.y;
}


void
Room::Draw(Bitmap *surface)
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (fWorldMap != NULL) {
		GFX::rect sourceRect = offset_rect(fViewPort,
				-fViewPort.x, -fViewPort.y);
		sourceRect = offset_rect(sourceRect, fAreaOffset.x, fAreaOffset.y);
		gfx->BlitToScreen(fWorldMapBitmap, &sourceRect, &fViewPort);
	} else {
		if (sSelectedActorRadius > 22) {
			sSelectedActorRadiusStep = -1;
		} else if (sSelectedActorRadius < 18) {
			sSelectedActorRadiusStep = 1;
		}
		sSelectedActorRadius += sSelectedActorRadiusStep;

		fBackBitmap->ClearColorKey();
		fBackBitmap->Clear(0);

		_DrawBaseMap();

		GFX::rect mapRect = offset_rect_to(fViewPort,
				fAreaOffset.x, fAreaOffset.y);

		if (fDrawAnimations)
			_DrawAnimations();

		_DrawActors();

		if (fDrawPolygons) {
			fBackBitmap->Lock();
			for (uint32 p = 0; p < fWed->CountPolygons(); p++) {
				const Polygon* poly = fWed->PolygonAt(p);
				if (poly != NULL && poly->CountPoints() > 0) {
					if (rects_intersect(poly->Frame(), mapRect)) {
						uint32 color = 0;
						if (poly->Flags() & IE::POLY_SHADE_WALL)
							color = 200;
						else if (poly->Flags() & IE::POLY_HOVERING)
							color = 500;
						else if (poly->Flags() & IE::POLY_COVER_ANIMATIONS)
							color = 1000;

						fBackBitmap->FillPolygon(*poly, color,
											-fAreaOffset.x, -fAreaOffset.y);
						fBackBitmap->StrokePolygon(*poly, color,
											-fAreaOffset.x, -fAreaOffset.y);
					}
				}
			}
			fBackBitmap->Unlock();
		}

		// TODO: handle this better
		if (Door* door = dynamic_cast<Door*>(fMouseOverObject)) {
			GFX::rect rect = rect_to_gfx_rect(door->Frame());
			rect = offset_rect(rect, -mapRect.x, -mapRect.y);
			fBackBitmap->Lock();
			fBackBitmap->StrokeRect(rect, 70);
			fBackBitmap->Unlock();
		} else if (Actor* actor = dynamic_cast<Actor*>(fMouseOverObject)) {
			try {
				GFX::rect rect = actor->Frame();
				rect = offset_rect(rect, -mapRect.x, -mapRect.y);
				fBackBitmap->Lock();
				IE::point position = offset_point(actor->Position(), -mapRect.x, -mapRect.y);
				uint32 color = fBackBitmap->MapColor(255, 255, 255);
				fBackBitmap->StrokeCircle(position.x, position.y, 20, color);
				fBackBitmap->Unlock();
			} catch (const char* string) {
				std::cerr << string << std::endl;
			} catch (...) {

			}
		} else if (Region* region = dynamic_cast<Region*>(fMouseOverObject)) {
			GFX::rect rect = rect_to_gfx_rect(region->Frame());
			rect = offset_rect(rect, -mapRect.x, -mapRect.y);

			fBackBitmap->Lock();
			uint32 color = 0;
			switch (region->Type()) {
				case IE::REGION_TYPE_TRAVEL:
					color = fBackBitmap->MapColor(0, 125, 0);
					break;
				case IE::REGION_TYPE_TRIGGER:
					color = fBackBitmap->MapColor(125, 0, 0);
					break;
				default:
					color = fBackBitmap->MapColor(255, 255, 255);
					break;
			}

			if (region->Polygon().CountPoints() > 2) {
				fBackBitmap->StrokePolygon(region->Polygon(), color,
									-mapRect.x, -mapRect.y);
			} else
				fBackBitmap->StrokeRect(rect, color);
			fBackBitmap->Unlock();
		}

		fBackBitmap->Update();
		gfx->BlitToScreen(fBackBitmap, NULL, &fViewPort);
		_DrawSearchMap(mapRect);
	}
}


void
Room::Clicked(uint16 x, uint16 y)
{
	IE::point point = {int16(x), int16(y)};
	ConvertToArea(point);

	if (fWorldMap != NULL) {
		for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
			AreaEntry& area = fWorldMap->AreaEntryAt(i);
			if (rect_contains(area.Rect(), point)) {
				LoadArea(area);
				break;
			}
		}
	} else {
		// TODO: Temporary, for testing
		if (Door* door = dynamic_cast<Door*>(fMouseOverObject)) {
			door->Toggle();
		} else 	if (Actor* actor = _ActorAtPoint(point)) {
			if (fSelectedActor != actor) {
				if (fSelectedActor != NULL)
					fSelectedActor->Select(false);
				fSelectedActor = actor;
				if (fSelectedActor != NULL)
					fSelectedActor->Select(true);
			}
		} else if (Region* region = _RegionAtPoint(point)) {
			if (region->Type() == IE::REGION_TYPE_TRAVEL) {
				LoadArea(region->DestinationArea(), "foo",
						region->DestinationEntrance());
			} else if (region->Type() == IE::REGION_TYPE_INFO) {
				uint32 strRef = region->InfoTextRef();
				TLKEntry* entry = Dialogs()->EntryAt(strRef);
				std::cout << entry->string << std::endl;
				delete entry;
			}
		} else if (fSelectedActor != NULL) {

			fSelectedActor->SetDestination(point);
		}
	}
}


void
Room::MouseOver(uint16 x, uint16 y)
{
	const uint16 kScrollingStep = 45;

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
	else if (x >= fViewPort.w - horizBorderSize)
		scrollByX = kScrollingStep;

	if (y <= vertBorderSize)
		scrollByY = -kScrollingStep;
	else if (y >= fViewPort.h - vertBorderSize)
		scrollByY = kScrollingStep;

	IE::point point = { int16(x), int16(y) };
	ConvertToArea(point);

	//std::cout << "state: " << std::dec << PointSearch(point) << std::endl;
	_UpdateCursor(x, y, scrollByX, scrollByY);

	// TODO: This screams for improvements
	if (fWed != NULL) {

		fMouseOverObject = _ObjectAtPoint(point);

	} else if (fWorldMap != NULL) {
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

	IE::point newAreaOffset = fAreaOffset;
	newAreaOffset.x += scrollByX;
	newAreaOffset.y += scrollByY;

	SetAreaOffset(newAreaOffset);
}



void
Room::DrawObject(const Object& object)
{
	if (const Actor* actor = dynamic_cast<const Actor*>(&object)) {
		IE::point actorPosition = actor->Position();
		if (actor->IsSelected()) {
			IE::point position = offset_point(actorPosition,
										-fAreaOffset.x, -fAreaOffset.y);

			fBackBitmap->Lock();
			uint32 color = fBackBitmap->MapColor(0, 255, 0);
			fBackBitmap->StrokeCircle(position.x, position.y,
										sSelectedActorRadius, color);
			if (actor->Destination() != actor->Position()) {
				IE::point destination = offset_point(actor->Destination(),
											-fAreaOffset.x, -fAreaOffset.y);
				fBackBitmap->StrokeCircle(
						destination.x, destination.y,
						sSelectedActorRadius - 10, color);
			}
			fBackBitmap->Unlock();
		}
		const Bitmap* actorFrame = actor->Bitmap();

		int32 pointHeight = PointHeight(actorPosition);
		actorPosition.y += pointHeight - 8;
		DrawObject(actorFrame, actorPosition);
	}
}


void
Room::DrawObject(const Bitmap* bitmap, const IE::point& point)
{
	if (bitmap == NULL)
		return;

	IE::point leftTop = offset_point(point,
							-(bitmap->Frame().x + bitmap->Frame().w / 2),
							-(bitmap->Frame().y + bitmap->Frame().h / 2));

	GFX::rect rect(leftTop.x, leftTop.y, bitmap->Width(), bitmap->Height());

	if (rects_intersect(fMapArea, rect)) {
		GFX::rect offsetRect = offset_rect(rect, -fAreaOffset.x, -fAreaOffset.y);
		GraphicsEngine::BlitBitmapWithMask(bitmap, NULL,
					fBackBitmap, &offsetRect, fBlitMask, &rect);
	}
}


uint16
Room::TileNumberForPoint(const IE::point& point)
{
	const uint16 overlayWidth = fOverlays[0]->Width();
	const uint16 tileX = point.x / TILE_WIDTH;
	const uint16 tileY = point.y / TILE_HEIGHT;

	return tileY * overlayWidth + tileX;
}


int32
Room::PointHeight(const IE::point& point) const
{
	if (fHeightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;
	uint8* pixels = (uint8*)fHeightMap->Pixels();
	return pixels[y * fHeightMap->Pitch() + x * fHeightMap->BitsPerPixel() / 8];
}


int32
Room::PointLight(const IE::point& point) const
{
	if (fLightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;
	uint8* pixels = (uint8*)fLightMap->Pixels();
	return pixels[y * fLightMap->Pitch() + x * fLightMap->BitsPerPixel() / 8];
}


int32
Room::PointSearch(const IE::point& point) const
{
	if (fSearchMap == NULL)
		return 1;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	uint8* pixels = (uint8*)fSearchMap->Pixels();
	return pixels[y * fSearchMap->Pitch() + x * fSearchMap->BitsPerPixel() / 8];
}


void
Room::ToggleOverlays()
{
	fDrawOverlays = !fDrawOverlays;
}


void
Room::TogglePolygons()
{
	fDrawPolygons = !fDrawPolygons;
}


void
Room::ToggleAnimations()
{
	fDrawAnimations = !fDrawAnimations;
}


void
Room::ToggleSearchMap()
{
	if (++fDrawSearchMap > 2)
		fDrawSearchMap = 0;

	if (fSearchMap == NULL)
		return;

	if (fDrawSearchMap == 2)
		fSearchMap->SetAlpha(127, true);
	else
		fSearchMap->SetAlpha(0, false);
}


void
Room::ToggleGUI()
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
Room::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
}


/* static */
Room*
Room::Get()
{
	return sCurrentRoom;
}


void
Room::_InitBitmap(GFX::rect area)
{
	GraphicsEngine::DeleteBitmap(fBackBitmap);
	fBackBitmap = GraphicsEngine::CreateBitmap(area.w, area.h, 16);
}


void
Room::_InitBlitMask()
{
	std::cout << "Initializing blit mask...";
	std::flush(std::cout);

	fBlitMask = GraphicsEngine::CreateBitmap(AreaRect().w, AreaRect().h, 8);

	fBlitMask->Lock();
	for (uint32 p = 0; p < fWed->CountPolygons(); p++) {
		const Polygon* poly = fWed->PolygonAt(p);
		uint32 mask = GraphicsEngine::MASK_COMPLETELY;
		if (poly->Flags() & IE::POLY_SHADE_WALL)
			mask = GraphicsEngine::MASK_SHADE;
		if (poly != NULL && poly->CountPoints() > 0) {
			fBlitMask->FillPolygon(*poly, mask);
		}
	}
	fBlitMask->Unlock();
	fBlitMask->Update();

	std::cout << "Done!" << std::endl;
}


void
Room::_InitHeightMap()
{
	std::string heightMapName = fArea->WedName().CString();
	heightMapName += "HT";
	BMPResource* resource = gResManager->GetBMP(heightMapName.c_str());
	if (resource != NULL) {
		fHeightMap = resource->Image();
		gResManager->ReleaseResource(resource);
	}
}


void
Room::_InitLightMap()
{
	std::string lightMapName = fArea->WedName().CString();
	lightMapName += "LM";
	BMPResource* resource = gResManager->GetBMP(lightMapName.c_str());
	if (resource != NULL) {
		fLightMap = resource->Image();
		gResManager->ReleaseResource(resource);
	}
}


void
Room::_InitSearchMap()
{
	std::string searchMapName = fArea->WedName().CString();
	searchMapName += "SR";
	BMPResource* resource = gResManager->GetBMP(searchMapName.c_str());
	if (resource != NULL) {
		fSearchMap = resource->Image();
		fMapHorizontalRatio = AreaRect().w / fSearchMap->Width();
		fMapVerticalRatio = AreaRect().h / fSearchMap->Height();
		gResManager->ReleaseResource(resource);
	}

}


void
Room::_DrawBaseMap()
{
	MapOverlay *overlay = fOverlays[0];
	if (overlay == NULL) {
		std::cerr << "Overlay 0 is NULL!!" << std::endl;
		return;
	}
	const uint16 overlayWidth = overlay->Width();
	const uint16 firstTileX = fAreaOffset.x / TILE_WIDTH;
	const uint16 firstTileY = fAreaOffset.y / TILE_HEIGHT;
	uint16 lastTileX = firstTileX + (fBackBitmap->Width() / TILE_WIDTH) + 2;
	uint16 lastTileY = firstTileY + (fBackBitmap->Height() / TILE_HEIGHT) + 2;

	lastTileX = std::min(lastTileX, overlayWidth);
	lastTileY = std::min(lastTileY, overlay->Height());

	GFX::rect tileRect(0, 0, TILE_WIDTH, TILE_HEIGHT);
	for (uint16 y = firstTileY; y < lastTileY; y++) {
		tileRect.y = y * TILE_HEIGHT - fAreaOffset.y;
		const uint32 tileNumY = y * overlayWidth;
		for (uint16 x = firstTileX; x < lastTileX; x++) {
			tileRect.x = x * TILE_WIDTH - fAreaOffset.x;
			TileCell* tile = fTileCells[tileNumY + x];
			//assert(tile != NULL);
			tile->Draw(fBackBitmap, &tileRect, fDrawOverlays);
		}
	}
}


void
Room::_DrawAnimations()
{
	if (fAnimations.size() == 0)
		return;

	for (uint32 i = 0; i < fArea->CountAnimations(); i++) {
		try {
			if (fAnimations[i] != NULL && fAnimations[i]->IsShown()) {
				const Bitmap* frame = fAnimations[i]->NextBitmap();

				DrawObject(frame, fAnimations[i]->Position());
			}
		} catch (const char* string) {
			std::cerr << string << std::endl;
			continue;
		} catch (...) {
			continue;
		}
	}
}


void
Room::_DrawActors()
{
	std::vector<Actor*>::iterator a;
	for (a = Actor::List().begin(); a != Actor::List().end(); a++) {
		try {
			DrawObject(*(*a));
		} catch (const char* string) {
			std::cerr << "_DrawActors: exception: " << string << std::endl;
			continue;
		} catch (...) {
			std::cerr << "Caught exception on actor " << (*a)->Name() << std::endl;
			continue;
		}
	}
}


void
Room::_DrawSearchMap(GFX::rect visibleArea)
{
	if (fSearchMap != NULL && fDrawSearchMap > 0) {
		GFX::rect destRect(0, fBackBitmap->Height() - fSearchMap->Height(),
							fBackBitmap->Width(), fBackBitmap->Height());

		GraphicsEngine::Get()->BlitToScreen(fSearchMap, NULL, &destRect);

		visibleArea.x /= fMapHorizontalRatio;
		visibleArea.y /= fMapVerticalRatio;
		visibleArea.w /= fMapHorizontalRatio;
		visibleArea.h /= fMapVerticalRatio;
		visibleArea = offset_rect(visibleArea, 0, fBackBitmap->Height() - fSearchMap->Height());
		GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(visibleArea, 500);
	}
}


void
Room::_UpdateCursor(int x, int y, int scrollByX, int scrollByY)
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


Actor*
Room::_ActorAtPoint(const IE::point& point)
{
	std::vector<Actor*>& actorList = Actor::List();
	std::vector<Actor*>::const_iterator iter;
	for (iter = actorList.begin(); iter != actorList.end(); iter++) {
		try {
			const GFX::rect actorFrame = (*iter)->Frame();
			if (rect_contains(actorFrame, point))
				return *iter;
		} catch (...) {
			continue;
		}
	}

	return NULL;
}


Region*
Room::_RegionAtPoint(const IE::point& point)
{
	std::vector<Region*>::const_iterator i;
	for (i = fRegions.begin(); i != fRegions.end(); i++) {
		IE::rect rect = (*i)->Frame();
		if (rect_contains(rect_to_gfx_rect(rect), point))
			return *i;
	}
	return NULL;
}


Object*
Room::_ObjectAtPoint(const IE::point& point)
{
	Object* object = NULL;
	const uint16 tileNum = TileNumberForPoint(point);
	if (tileNum >= fTileCells.size())
		return object;

	Door* door = fTileCells[tileNum]->Door();
	if (door != NULL && rect_contains(door->Frame(), point)) {
		object = door;
	} else if (Region* region = _RegionAtPoint(point)) {
		object = region;
		// TODO: This is a side effect of a method
		// which should just return an object, and in
		// fact _ObjectAtPoint() should be const.
		GUI::Get()->SetCursor(region->CursorIndex());
	} else {
		object = _ActorAtPoint(point);
	}

	return object;
}


void
Room::_LoadOverlays()
{
	std::cout << "Loading Overlays...";
	std::flush(std::cout);
	uint32 numOverlays = fWed->CountOverlays();
	for (uint32 i = 0; i < numOverlays; i++) {
		MapOverlay *overlay = fWed->GetOverlay(i);
		fOverlays.push_back(overlay);
	}
	std::cout << "Done! Loaded " << numOverlays << " overlays. ";
	std::cout << "Map size: " << fOverlays[0]->Width();
	std::cout << "x" << fOverlays[0]->Height() << std::endl;
}


void
Room::_InitTileCells()
{
	std::cout << "Initializing Tile Cells...";
	std::flush(std::cout);
	uint32 numTiles = fOverlays[0]->Size();
	for (uint16 i = 0; i < numTiles; i++) {
		fTileCells.push_back(new TileCell(i, fOverlays.data(), fOverlays.size()));
	}
	std::cout << "Done! Loaded " << numTiles << " tile cells!" << std::endl;
}


void
Room::_InitVariables()
{
	std::cout << "Initializing Variables...";
	std::flush(std::cout);

	uint32 numVars = fArea->CountVariables();
	for (uint32 n = 0; n < numVars; n++) {
		IE::variable var = fArea->VariableAt(n);
		Core::Get()->SetVariable(var.name, var.value);
	}
	std::cout << "Done!" << std::endl;
}


void
Room::_InitAnimations()
{
	std::cout << "Initializing Animations...";
	std::flush(std::cout);
	for (uint32 i = 0; i < fArea->CountAnimations(); i++)
		fAnimations.push_back(new Animation(fArea->AnimationAt(i)));
	std::cout << "Done!" << std::endl;
}


void
Room::_InitRegions()
{
	std::cout << "Initializing Regions...";
	std::flush(std::cout);

	for (uint16 i = 0; i < fArea->CountRegions(); i++) {
		fRegions.push_back(fArea->GetRegionAt(i));
	}
	std::cout << "Done!" << std::endl;
}


void
Room::_LoadActors()
{
	std::cout << "Loading Actors...";
	std::flush(std::cout);

	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		Actor::Add(fArea->GetActorAt(i));
	}
	std::cout << "Done!" << std::endl;
}


void
Room::_InitDoors()
{
	std::cout << "Initializing Doors...";
	std::flush(std::cout);

	assert(fTileCells.size() > 0);

	const uint32 numDoors = fWed->CountDoors();
	for (uint32 c = 0; c < numDoors; c++) {
		Door *door = new Door(fArea->DoorAt(c));
		fWed->GetDoorTiles(door, c);
		Door::Add(door);

		for (uint32 i = 0; i < door->fTilesOpen.size(); i++) {
			fTileCells[door->fTilesOpen[i]]->SetDoor(door);
		}
	}
	std::cout << "Done!" << std::endl;
}


void
Room::_UnloadArea()
{
	if (fWed == NULL)
		return;

	if (fSelectedActor != NULL) {
		fSelectedActor->Select(false);
		fSelectedActor = NULL;
	}

	if (fMouseOverObject != NULL)
	    fMouseOverObject = NULL;
	
	for (uint32 c = 0; c < fRegions.size(); c++)
		delete fRegions[c];
	fRegions.clear();

	for (uint32 c = 0; c < fAnimations.size(); c++)
		delete fAnimations[c];
	fAnimations.clear();

	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();

	std::vector<Actor*>::const_iterator actorIter;
	for (actorIter = Actor::List().begin();
			actorIter != Actor::List().end();
			actorIter++) {
		delete *actorIter;
	}
	Actor::List().clear();

	for (uint32 c = 0; c < fOverlays.size(); c++)
		delete fOverlays[c];
	fOverlays.clear();

	gResManager->ReleaseResource(fWed);
	fWed = NULL;
	gResManager->ReleaseResource(fArea);
	fArea = NULL;
	gResManager->ReleaseResource(fBcs);
	fBcs = NULL;
	GraphicsEngine::DeleteBitmap(fHeightMap);
	fHeightMap = NULL;
	GraphicsEngine::DeleteBitmap(fLightMap);
	fLightMap = NULL;
	GraphicsEngine::DeleteBitmap(fSearchMap);
	fSearchMap = NULL;

	gResManager->TryEmptyResourceCache();
}


void
Room::_UnloadWorldMap()
{
	if (fWorldMap == NULL)
		return;

	gResManager->ReleaseResource(fWorldMap);
	fWorldMap = NULL;
	gResManager->ReleaseResource(fWorldMapBackground);
	fWorldMapBackground = NULL;
	GraphicsEngine::DeleteBitmap(fWorldMapBitmap);
	fWorldMapBitmap = NULL;

	gResManager->TryEmptyResourceCache();
}
