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
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "TileCell.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <assert.h>
#include <iostream>

std::vector<MapOverlay*> *gOverlays = NULL;

static Room* sCurrentRoom = NULL;

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
Room::LoadArea(const res_ref& areaName, const char* longName)
{
	_UnloadWorldMap();

	fAreaOffset.x = fAreaOffset.y = 0;
	fName = areaName;

	std::cout << "Room::Load(" << areaName.CString() << ")" << std::endl;

	fArea = gResManager->GetARA(fName);
	if (fArea == NULL)
		return false;

	fWed = gResManager->GetWED(fName);
	if (fWed == NULL)
		return false;

	fBcs = gResManager->GetBCS(fArea->ScriptName());
	Script* roomScript = NULL;
	if (fBcs != NULL)
		roomScript = fBcs->GetScript();

	GUI::Default()->Clear();
	GUI::Default()->Load("GUIMAP");
	GUI::Default()->GetWindow(0);
	GUI::Default()->GetWindow(1);
	Window* window = GUI::Default()->GetWindow(2);
	//GUI::Default()->GetWindow(3);
	//GUI::Default()->GetWindow(4);

	window->GetControlByID(2)->AssociateRoom(this);
	Label* label = dynamic_cast<Label*>(window->GetControlByID(268435459));
	if (label != NULL)
		label->SetText(longName);

	_LoadOverlays();
	_InitTileCells();
	_InitVariables();
	_InitAnimations();
	_LoadActors();
	_InitDoors();

	_InitBitmap(fViewPort);

	Core::Get()->EnteredArea(this, roomScript);

	delete roomScript;
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
	GUI* gui = GUI::Default();

	gui->Clear();
	gui->Load("GUIWMAP");

	Window* window = gui->GetWindow(0);

	Control* control = window->GetControlByID(4);
	control->AssociateRoom(this);

	// TODO: _UnloadActors/Doors() (if needed)
	if (fWorldMap != NULL)
		return true;

	_UnloadArea();

	fAreaOffset.x = fAreaOffset.y = 0;

	Core::Get()->EnteredArea(this, NULL);

	fName = "WORLDMAP";
	fWorldMap = gResManager->GetWMAP(fName);

	worldmap_entry entry = fWorldMap->WorldMapEntry();
	fWorldMapBackground = gResManager->GetMOS(entry.background_mos);
	fWorldMapBitmap = fWorldMapBackground->Image();
	for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
		AreaEntry& areaEntry = fWorldMap->AreaEntryAt(i);
		const Frame& iconFrame = areaEntry.Icon();
		IE::point position = areaEntry.Position();
		GFX::rect iconRect = { position.x - iconFrame.rect.w / 2,
					position.y - iconFrame.rect.h / 2,
					iconFrame.rect.w, iconFrame.rect.h };

		GraphicsEngine::Get()->BlitBitmap(iconFrame.bitmap, NULL,
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


void
Room::GetAreaOffset(IE::point& point)
{
	point = fAreaOffset;
}


void
Room::SetAreaOffset(IE::point point)
{
	GFX::rect areaRect = AreaRect();
	fAreaOffset = point;
	if (fAreaOffset.x < 0)
		fAreaOffset.x = 0;
	else if (fAreaOffset.x + fViewPort.w > areaRect.w)
		fAreaOffset.x = areaRect.w - fViewPort.w;
	if (fAreaOffset.y < 0)
		fAreaOffset.y = 0;
	else if (fAreaOffset.y + fViewPort.h > areaRect.h)
		fAreaOffset.y = areaRect.h - fViewPort.h;
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

	//gfx->SetClipping(&fViewPort);

	if (fWorldMap != NULL) {
		GFX::rect sourceRect = offset_rect(fViewPort,
				-fViewPort.x, -fViewPort.y);
		sourceRect = offset_rect(sourceRect, fAreaOffset.x, fAreaOffset.y);
		gfx->BlitToScreen(fWorldMapBitmap, &sourceRect, &fViewPort);
	} else {
		fBackBitmap->ClearColorKey();
		fBackBitmap->Clear(0);

		_DrawBaseMap();

		GFX::rect mapRect = offset_rect_to(fViewPort,
				fAreaOffset.x, fAreaOffset.y);

		if (fDrawAnimations)
			_DrawAnimations(mapRect);

		if (true)
			_DrawActors(mapRect);

		/*if (fDrawPolygons) {
			for (uint32 p = 0; p < fWed->CountPolygons(); p++) {
				Polygon* poly = fWed->PolygonAt(p);
				if (poly != NULL && poly->CountPoints() > 0) {
					if (rects_intersect(offset_rect(poly->Frame(),
							-mapRect.x, -mapRect.y), mapRect)) {
						Graphics::DrawPolygon(*poly, SDL_GetVideoSurface(),
								-mapRect.x, -mapRect.y);
					}
				}
			}
		}*/

		fBackBitmap->Update();
		GraphicsEngine::Get()->BlitToScreen(fBackBitmap, NULL, &fViewPort);
	}
	//gfx->SetClipping(NULL);
}


void
Room::Clicked(uint16 x, uint16 y)
{
	IE::point point = {x, y};
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
		const uint16 tileNum = TileNumberForPoint(point);
		fTileCells[tileNum]->Clicked();
	}
}


void
Room::MouseOver(uint16 x, uint16 y)
{
	const uint16 kBorderSize = 25;
	const uint16 kScrollingStep = 20;

	uint16 scrollByX = 0;
	uint16 scrollByY = 0;
	if (x <= kBorderSize)
		scrollByX = -kScrollingStep;
	else if (x >= fViewPort.w - kBorderSize)
		scrollByX = kScrollingStep;

	if (y <= kBorderSize)
		scrollByY = -kScrollingStep;
	else if (y >= fViewPort.h - kBorderSize)
		scrollByY = kScrollingStep;

	IE::point point = { x, y };
	ConvertToArea(point);

	if (fWed != NULL) {
		const uint16 tileNum = TileNumberForPoint(point);

		fTileCells[tileNum]->MouseOver();
	} else if (fWorldMap != NULL) {
		for (uint32 i = 0; i < fWorldMap->CountAreaEntries(); i++) {
			AreaEntry& area = fWorldMap->AreaEntryAt(i);
			GFX::rect areaRect = area.Rect();
			if (rect_contains(areaRect, point)) {
				ConvertFromArea(areaRect);
				ConvertToScreen(areaRect);
				//GUI::Default()->GetWindow(0)->;
				char* toolTip = area.TooltipName();
				//RenderString(toolTip, GraphicsEngine::Get()->ScreenSurface());
				free(toolTip);
				GraphicsEngine::Get()->StrokeRect(areaRect, 600);
				break;
			}
		}
	}

	IE::point newAreaOffset = fAreaOffset;
	newAreaOffset.x += scrollByX;
	newAreaOffset.y += scrollByY;

	SetAreaOffset(newAreaOffset);
}


uint16
Room::TileNumberForPoint(const IE::point& point)
{
	const uint16 overlayWidth = fOverlays[0]->Width();
	const uint16 tileX = point.x / TILE_WIDTH;
	const uint16 tileY = point.y / TILE_HEIGHT;

	return tileY * overlayWidth + tileX;
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


/* virtual */
void
Room::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect = {0, 0, width, height};
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
Room::_DrawBaseMap()
{
	MapOverlay *overlay = fOverlays[0];
	const uint16 overlayWidth = overlay->Width();
	const uint16 firstTileX = fAreaOffset.x / TILE_WIDTH;
	const uint16 firstTileY = fAreaOffset.y / TILE_HEIGHT;
	uint16 lastTileX = firstTileX + (fBackBitmap->Width() / TILE_WIDTH) + 2;
	uint16 lastTileY = firstTileY + (fBackBitmap->Height() / TILE_HEIGHT) + 2;

	lastTileX = std::min(lastTileX, overlayWidth);
	lastTileY = std::min(lastTileY, overlay->Height());

	GFX::rect tileRect = {
			0, 0,
			TILE_WIDTH,
			TILE_HEIGHT
	};
	for (uint16 y = firstTileY; y < lastTileY; y++) {
		tileRect.y = y * TILE_HEIGHT - fAreaOffset.y;
		const uint32 tileNumY = y * overlayWidth;
		for (uint16 x = firstTileX; x < lastTileX; x++) {
			tileRect.x = x * TILE_WIDTH - fAreaOffset.x;
			const uint32 tileNum = tileNumY + x;
			fTileCells[tileNum]->Draw(fBackBitmap, &tileRect, fDrawOverlays);
		}

	}
}


void
Room::_DrawAnimations(GFX::rect mapArea)
{
	if (fAnimations.size() == 0)
		return;

	for (uint32 i = 0; i < fArea->CountAnimations(); i++) {
		if (fAnimations[i] != NULL && fAnimations[i]->IsShown()) {
			Frame frame = fAnimations[i]->NextFrame();
			IE::point center = fAnimations[i]->Position();
			center = offset_point(center, -frame.rect.w / 2,
					-frame.rect.h / 2);
			Bitmap *animImage = frame.bitmap;
			if (animImage == NULL)
				continue;

			GFX::rect rect = { center.x, center.y,
					animImage->Width(), animImage->Height() };

			rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);
			if (rects_intersect(mapArea, rect)) {
				rect = offset_rect(rect, -fAreaOffset.x, -fAreaOffset.y);
				GraphicsEngine::BlitBitmap(animImage, NULL, fBackBitmap, &rect);
			}
			GraphicsEngine::DeleteBitmap(frame.bitmap);
		}
	}
}


void
Room::_DrawActors(GFX::rect area)
{
	std::vector<Actor*>::iterator a;
	for (a = Actor::List().begin(); a != Actor::List().end(); a++) {
		try {
			(*a)->Draw(area, fBackBitmap);
		} catch (...) {
			continue;
		}
	}
}


void
Room::_LoadOverlays()
{
	uint32 numOverlays = fWed->CountOverlays();
	for (uint32 i = 0; i < numOverlays; i++) {
		MapOverlay *overlay = fWed->GetOverlay(i);
		fOverlays.push_back(overlay);
	}
}


void
Room::_InitTileCells()
{
	uint32 numTiles = fOverlays[0]->Size();
	for (uint16 i = 0; i < numTiles; i++) {
		fTileCells.push_back(new TileCell(i, fOverlays.data(), fOverlays.size()));
	}
}


void
Room::_InitVariables()
{
	uint32 numVars = fArea->CountVariables();
	for (uint32 n = 0; n < numVars; n++) {
		IE::variable var = fArea->VariableAt(n);
		Core::Get()->SetVariable(var.name, var.value);
	}
}


void
Room::_InitAnimations()
{
	for (uint32 i = 0; i < fArea->CountAnimations(); i++)
		fAnimations.push_back(new Animation(fArea->AnimationAt(i)));
}


void
Room::_LoadActors()
{
	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		Actor::Add(new Actor(*fArea->ActorAt(i)));
	}
}


void
Room::_InitDoors()
{
	assert(fTileCells.size() > 0);

	uint32 numDoors = fWed->CountDoors();
	for (uint32 c = 0; c < numDoors; c++) {
		Door *door = new Door(fArea->DoorAt(c));
		fWed->GetDoorTiles(door, c);
		Door::Add(door);

		for (uint32 i = 0; i < door->fTilesOpen.size(); i++) {
			fTileCells[door->fTilesOpen[i]]->SetDoor(door);
		}
	}
}


void
Room::_UnloadArea()
{
	if (fWed == NULL)
		return;

	for (uint32 c = 0; c < fAnimations.size(); c++)
		delete fAnimations[c];
	fAnimations.clear();

	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();

	for (uint32 c = 0; c < fOverlays.size(); c++)
		delete fOverlays[c];
	fOverlays.clear();

	std::vector<Actor*>::iterator i;

	Actor::List().erase(Actor::List().begin(), Actor::List().end());

	gResManager->ReleaseResource(fWed);
	fWed = NULL;
	gResManager->ReleaseResource(fArea);
	fArea = NULL;
	gResManager->ReleaseResource(fBcs);
	fBcs = NULL;

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
