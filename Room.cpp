#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "BmpResource.h"
#include "Bitmap.h"
#include "Container.h"
#include "Control.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Game.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "InputConsole.h"
#include "Label.h"
#include "MOSResource.h"
#include "OutputConsole.h"
#include "Party.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "TextArea.h"
#include "TileCell.h"
#include "TisResource.h"
#include "Timer.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include <SDL.h>

static RoomContainer* sCurrentRoom = NULL;

static int sSelectedActorRadius = 20;
static int sSelectedActorRadiusStep = 1;

RoomContainer::RoomContainer()
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
}


RoomContainer::~RoomContainer()
{
	// TODO: Delete various tilecells, overlays, animations
	_Unload();

	if (fBackBitmap != NULL)
		fBackBitmap->Release();
	if (fBlitMask != NULL)
		fBlitMask->Release();
}


/* static */
bool
RoomContainer::Create()
{
	if (sCurrentRoom == NULL)
		sCurrentRoom = new RoomContainer();
	return true;
}


void
RoomContainer::Delete()
{
	std::cout << "Room::Delete()" << std::endl;
	delete sCurrentRoom;
	sCurrentRoom = NULL;	
}


void
RoomContainer::StartLoop(bool executeScripts)
{
	uint16 lastMouseX = 0;
	uint16 lastMouseY = 0;

	SDL_EnableUNICODE(1);

	GFX::rect screenRect = GraphicsEngine::Get()->ScreenFrame();
	GUI* gui = GUI::Get();
	GFX::rect consoleRect(
			0,
			0,
			screenRect.w,
			screenRect.h - 21);
	OutputConsole* console = new OutputConsole(consoleRect);
	consoleRect.h = 20;
	consoleRect.y = screenRect.h - consoleRect.h;
	InputConsole* inputConsole = new InputConsole(consoleRect);
	inputConsole->Initialize();

	SDL_Event event;
	bool quitting = false;
	while (!quitting) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					//downMouseX = event.button.x;
					//downMouseY = event.button.y;
					gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					/*if (downMouseX == event.button.x
						&& downMouseY == event.button.y)
						map->Clicked(event.button.x, event.button.y);*/
					gui->MouseUp(event.button.x, event.button.y);

					break;
				case SDL_MOUSEMOTION:
					lastMouseX = event.motion.x;
					lastMouseY = event.motion.y;

					break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						console->Toggle();
						inputConsole->Toggle();
					} else if (console->IsActive()) {
						if (event.key.keysym.unicode < 0x80 && event.key.keysym.unicode > 0) {
							uint8 key = event.key.keysym.sym;
							if (event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
								key -= 32;
							inputConsole->HandleInput(key);
						}
					} else {
						switch (event.key.keysym.sym) {
							case SDLK_o:
								ToggleOverlays();
								break;

							// TODO: Move to GUI class
							case SDLK_h:
								ToggleGUI();
								break;
							case SDLK_a:
								ToggleAnimations();
								break;
							case SDLK_p:
								TogglePolygons();
								break;
							case SDLK_w:
								LoadWorldMap();
								break;
							case SDLK_s:
								ToggleSearchMap();
								break;
							case SDLK_n:
								ToggleDayNight();
								break;
							case SDLK_q:
								quitting = true;
								break;
							case SDLK_1:
								GUI::Get()->ToggleWindow(1);
								break;
							case SDLK_2:
								GUI::Get()->ToggleWindow(2);
								break;
							case SDLK_3:
								GUI::Get()->ToggleWindow(3);
								break;
							case SDLK_4:
								GUI::Get()->ToggleWindow(4);
								break;
							case SDLK_SPACE:
								Core::Get()->TogglePause();
								break;
							default:
								break;
						}
					}
				}
				break;

				case SDL_QUIT:
					quitting = true;
					break;
				default:
					break;
			}
		}

		// TODO: When MouseOver() doesn't draw anymore, reorder
		// these three calls. Draw() should be the last.
		gui->Draw();

		// TODO: needs to be called at every loop, not only when the mouse
		// is moved
		gui->MouseMoved(lastMouseX, lastMouseY);

		console->Draw();
		inputConsole->Draw();
		Core::Get()->UpdateLogic(executeScripts);

		GraphicsEngine::Get()->Flip();
		if (inputConsole->IsActive())
			Timer::Wait(25);
		else
			Timer::Wait(50);
	}

	delete console;
	delete inputConsole;
}


res_ref
RoomContainer::AreaName() const
{
	return fName;
}


WEDResource*
RoomContainer::WED()
{
	return fWed;
}


ARAResource*
RoomContainer::AREA() const
{
	return fArea;
}


GFX::rect
RoomContainer::ViewPort() const
{
	return fViewPort;
}


bool
RoomContainer::LoadArea(const res_ref& areaName, const char* longName,
					const char* entranceName)
{
	// Save the entrance name, it will be unloaded in UnloadArea
	std::string savedEntranceName = entranceName ? entranceName : "";

	_Unload();

	fName = areaName;

	GraphicsEngine::Get()->SetWindowCaption(fName.CString());

	std::cout << "Room::Load(" << areaName.CString() << ")" << std::endl;

	fArea = gResManager->GetARA(fName);
	if (fArea == NULL)
		return false;

	_InitWed(fArea->WedName().CString());

	fBcs = gResManager->GetBCS(fArea->ScriptName());
	Script* roomScript = NULL;
	if (fBcs != NULL)
		roomScript = fBcs->GetScript();

	SetScript(roomScript);

	GUI* gui = GUI::Get();
	gui->Clear();

	if (!gui->Load("GUIW")) {
		// TODO: Delete other loaded stuff
		gResManager->ReleaseResource(fArea);
		fArea = NULL;
		gResManager->ReleaseResource(fBcs);
		fBcs = NULL;
		SetScript(NULL);
		delete roomScript;
		return false;
	}

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

	_InitVariables();
	_InitAnimations();
	_InitRegions();
	_LoadActors();
	_InitDoors();
	_InitContainers();

	_InitBitmap(fViewPort);

	_InitBlitMask();

	Core::Get()->EnteredArea(this, roomScript);

	delete roomScript;

	IE::point point = { 0, 0 };
	if (!savedEntranceName.empty()) {
		for (uint32 e = 0; e < fArea->CountEntrances(); e++) {
			IE::entrance entrance = fArea->EntranceAt(e);
			//std::cout << "current: " << entrance.name;
			//std::cout << ", looking for " << entranceName << std::endl;

			if (savedEntranceName == entrance.name) {
				point.x = entrance.x;
				point.y = entrance.y;
				CenterArea(point);
				break;
			}
		}
	} else {
		try {
			IE::entrance entrance = fArea->EntranceAt(0);
			point.x = entrance.x;
			point.y = entrance.y;
		} catch (std::out_of_range& ex) {

		}
		CenterArea(point);
	}

	Actor* player = Game::Get()->Party()->ActorAt(0);
	if (player != NULL) {
		player->SetPosition(point);
		if (!player->IsSelected())
			player->Select(true);
		fSelectedActor = player;
	}


	//GUI::Get()->DrawTooltip("THIS IS A TEXT", 50, 40, 3000);

	return true;
}


bool
RoomContainer::LoadArea(AreaEntry& area)
{
	//MOSResource* mos = gResManager->GetMOS(area.LoadingScreenName());
	/*MOSResource* mos = gResManager->GetMOS("BACK");
	if (mos != NULL) {
		Bitmap* loadingScreen = mos->Image();
		if (loadingScreen != NULL) {
			GraphicsEngine::Get()->BlitToScreen(loadingScreen, NULL, NULL);
			GraphicsEngine::Get()->Flip();
			SDL_Delay(2000);
			loadingScreen->Release();
		}
		gResManager->ReleaseResource(mos);
	}*/

	bool result = LoadArea(area.Name(), area.LongName());

	return result;
}


bool
RoomContainer::LoadWorldMap()
{
	if (fWorldMap != NULL)
		return true;

	_Unload();

	GUI* gui = GUI::Get();

	gui->Clear();
	
	if (!gui->Load("GUIWMAP")) {
		return false;
	}

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
RoomContainer::SetViewPort(GFX::rect rect)
{
	fViewPort = rect;
}


GFX::rect
RoomContainer::AreaRect() const
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
RoomContainer::AreaOffset() const
{
	return fAreaOffset;
}


void
RoomContainer::SetAreaOffset(IE::point point)
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
RoomContainer::SetRelativeAreaOffset(IE::point relativePoint)
{
	IE::point newOffset = fAreaOffset;
	newOffset.x += relativePoint.x;
	newOffset.y += relativePoint.y;
	SetAreaOffset(newOffset);
}


void
RoomContainer::CenterArea(const IE::point& point)
{
	IE::point destPoint;
	destPoint.x = point.x - fViewPort.w / 2;
	destPoint.y = point.y - fViewPort.y / 2;
	SetAreaOffset(destPoint);
}


void
RoomContainer::ConvertToArea(GFX::rect& rect)
{
	rect.x += fAreaOffset.x;
	rect.y += fAreaOffset.y;
}


void
RoomContainer::ConvertToArea(IE::point& point)
{
	point.x += fAreaOffset.x;
	point.y += fAreaOffset.y;
}


void
RoomContainer::ConvertFromArea(GFX::rect& rect)
{
	rect.x -= fAreaOffset.x;
	rect.y -= fAreaOffset.y;
}


void
RoomContainer::ConvertFromArea(IE::point& point)
{
	point.x -= fAreaOffset.x;
	point.y -= fAreaOffset.y;
}


void
RoomContainer::ConvertToScreen(GFX::rect& rect)
{
	rect.x += fViewPort.x;
	rect.y += fViewPort.y;
}


void
RoomContainer::ConvertToScreen(IE::point& point)
{
	point.x += fViewPort.x;
	point.y += fViewPort.y;
}


void
RoomContainer::Draw(Bitmap *surface)
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (fWorldMap != NULL) {
		GFX::rect sourceRect = offset_rect(fViewPort,
				-fViewPort.x, -fViewPort.y);
		sourceRect = offset_rect(sourceRect, fAreaOffset.x, fAreaOffset.y);
		if (sourceRect.w < gfx->ScreenFrame().w || sourceRect.h < gfx->ScreenFrame().h) {
			GFX::rect clippingRect = fViewPort;
			clippingRect.w = gfx->ScreenFrame().w;
			clippingRect.h = gfx->ScreenFrame().h;
			gfx->SetClipping(&clippingRect);
			gfx->ScreenBitmap()->Clear(0);
			gfx->SetClipping(NULL);
		}
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

		GFX::rect mapRect = offset_rect_to(fViewPort,
				fAreaOffset.x, fAreaOffset.y);

		_DrawBaseMap(mapRect);

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
		if (Door* door = dynamic_cast<Door*>(fMouseOverObject.Target())) {
			GFX::rect rect = rect_to_gfx_rect(door->Frame());
			rect = offset_rect(rect, -mapRect.x, -mapRect.y);
			fBackBitmap->Lock();
			fBackBitmap->StrokeRect(rect, 70);
			fBackBitmap->Unlock();
		} else if (Actor* actor = dynamic_cast<Actor*>(fMouseOverObject.Target())) {
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
		} else if (Region* region = dynamic_cast<Region*>(fMouseOverObject.Target())) {
			GFX::rect rect = rect_to_gfx_rect(region->Frame());
			rect = offset_rect(rect, -mapRect.x, -mapRect.y);

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

			fBackBitmap->Lock();

			if (region->Polygon().CountPoints() > 2) {
				fBackBitmap->StrokePolygon(region->Polygon(), color,
									-mapRect.x, -mapRect.y);
			} else
				fBackBitmap->StrokeRect(rect, color);
			fBackBitmap->Unlock();
		} else if (Container* container = dynamic_cast<Container*>(fMouseOverObject.Target())) {
			uint32 color = 0;
			color = fBackBitmap->MapColor(0, 125, 0);
			// TODO: Different colors for trapped/nontrapped
			fBackBitmap->Lock();

			if (container->Polygon().CountPoints() > 2) {
						fBackBitmap->StrokePolygon(container->Polygon(), color,
											-mapRect.x, -mapRect.y);
			}
			fBackBitmap->Unlock();
		}

		fBackBitmap->Update();
		gfx->BlitToScreen(fBackBitmap, NULL, &fViewPort);
		_DrawSearchMap(mapRect);
	}
}


void
RoomContainer::Clicked(uint16 x, uint16 y)
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
		return;
	}

	if (fSelectedActor != NULL)
		fSelectedActor.Target()->ClearActionList();

	// TODO: Temporary, for testing
	if (Door* door = dynamic_cast<Door*>(fMouseOverObject.Target())) {
		if (fSelectedActor != NULL)
			fSelectedActor.Target()->ClickedOn(door);
		return;
	}

	if (Actor* actor = _ActorAtPoint(point)) {
		if (fSelectedActor != actor) {
			/*if (fSelectedActor != NULL)
				fSelectedActor->Select(false);
			fSelectedActor = actor;
			if (fSelectedActor != NULL)
				fSelectedActor->Select(true);
			*/
			if (fSelectedActor != NULL)
				fSelectedActor.Target()->ClickedOn(actor);

		}
		return;
	}

	if (Region* region = _RegionAtPoint(point)) {
		// TODO:
		if (fSelectedActor != NULL)
			fSelectedActor.Target()->ClickedOn(region);
		if (region->Type() == IE::REGION_TYPE_TRAVEL) {
			LoadArea(region->DestinationArea(), "foo",
					region->DestinationEntrance());
			return;
		} else if (region->Type() == IE::REGION_TYPE_INFO) {
			int32 strRef = region->InfoTextRef();
			if (strRef >= 0)
				Core::Get()->DisplayMessage(strRef);
			return;
		}
	} else if (Container* container = _ContainerAtPoint(point)) {
		// TODO:
		if (fSelectedActor != NULL)
			fSelectedActor.Target()->ClickedOn(container);
	}

	if (fSelectedActor != NULL) {
		WalkTo* walkTo = new WalkTo(fSelectedActor.Target(), point);
		fSelectedActor.Target()->AddAction(walkTo);
	}

}


void
RoomContainer::MouseOver(uint16 x, uint16 y)
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
	else if (x >= fViewPort.w - horizBorderSize)
		scrollByX = kScrollingStep;

	if (y <= vertBorderSize)
		scrollByY = -kScrollingStep;
	else if (y >= fViewPort.h - vertBorderSize)
		scrollByY = kScrollingStep;

	IE::point point = { int16(x), int16(y) };
	ConvertToArea(point);

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
RoomContainer::DrawObject(const Object& object)
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
		DrawObject(actorFrame, actorPosition, true);
	}
}


void
RoomContainer::DrawObject(const Bitmap* bitmap, const IE::point& point, bool mask)
{
	if (bitmap == NULL)
		return;

	IE::point leftTop = offset_point(point,
							-(bitmap->Frame().x + bitmap->Frame().w / 2),
							-(bitmap->Frame().y + bitmap->Frame().h / 2));

	GFX::rect rect(leftTop.x, leftTop.y, bitmap->Width(), bitmap->Height());

	if (rects_intersect(fMapArea, rect)) {
		GFX::rect offsetRect = offset_rect(rect, -fAreaOffset.x, -fAreaOffset.y);
		if (mask)
			GraphicsEngine::BlitBitmapWithMask(bitmap, NULL,
					fBackBitmap, &offsetRect, fBlitMask, &rect);
		else
			GraphicsEngine::BlitBitmap(bitmap, NULL, fBackBitmap, &offsetRect);
	}
}


void
RoomContainer::ActorEnteredArea(const Actor* actor)
{
	//fActors.push_back(const_cast<Actor*>(actor));
}


void
RoomContainer::ActorExitedArea(const Actor* actor)
{
	return;
	/*std::vector<Actor*>::iterator i =
			std::find(fActors.begin(), fActors.end(), actor);
	if (i != fActors.end()) {
		fActors.erase(i);
	}*/
}


uint16
RoomContainer::TileNumberForPoint(const IE::point& point)
{
	const uint16 overlayWidth = fOverlays[0]->Width();
	const uint16 tileX = point.x / TILE_WIDTH;
	const uint16 tileY = point.y / TILE_HEIGHT;

	return tileY * overlayWidth + tileX;
}


uint8
RoomContainer::PointHeight(const IE::point& point) const
{
	if (fHeightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	return (uint8)fHeightMap->GetPixel(x, y);
}


uint8
RoomContainer::PointLight(const IE::point& point) const
{
	if (fLightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	return (uint8)fLightMap->GetPixel(x, y);
}


uint8
RoomContainer::PointSearch(const IE::point& point) const
{
	if (fSearchMap == NULL)
		return 1;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	return (uint8)fSearchMap->GetPixel(x, y);
}


/* static */
bool
RoomContainer::IsPointPassable(const IE::point& point)
{
	int32 state = RoomContainer::Get()->PointSearch(point);
	switch (state) {
		case 0:
		case 8:
		case 10:
		case 12:
		case 13:
			return false;
		default:
			return true;
	}
}


void
RoomContainer::ToggleOverlays()
{
	fDrawOverlays = !fDrawOverlays;
}


void
RoomContainer::TogglePolygons()
{
	fDrawPolygons = !fDrawPolygons;
}


void
RoomContainer::ToggleAnimations()
{
	fDrawAnimations = !fDrawAnimations;
}


void
RoomContainer::ToggleSearchMap()
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
RoomContainer::ToggleGUI()
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


void
RoomContainer::ToggleDayNight()
{
	if (fWorldMap != NULL)
		return;

	std::string wedName = fWed->Name();
	if (*wedName.rbegin() == 'N')
		wedName = fArea->Name();
	else
		wedName.append("N");

	std::vector<std::string> list;
	if (gResManager->GetResourceList(list, wedName.c_str(), RES_WED) > 0)
		_InitWed(wedName.c_str());
}


TileCell*
RoomContainer::TileAt(uint16 x, uint16 y) const
{
	return fTileCells[y + x];
}


/* virtual */
void
RoomContainer::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
}


/* static */
RoomContainer*
RoomContainer::Get()
{
	return sCurrentRoom;
}


void
RoomContainer::_InitBitmap(GFX::rect area)
{
	if (fBackBitmap != NULL)
		fBackBitmap->Release();
	fBackBitmap = new Bitmap(area.w, area.h, 16);
}


void
RoomContainer::_InitWed(const char* name)
{
	// TODO: Assume that the various resources have
	// already been deleted and remove these lines ?
	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();

	for (uint32 c = 0; c < fOverlays.size(); c++)
		delete fOverlays[c];
	fOverlays.clear();

	gResManager->ReleaseResource(fWed);

	fWed = gResManager->GetWED(name);

	_LoadOverlays();
	_InitTileCells();

	_InitHeightMap();
	_InitLightMap();
	_InitSearchMap();
}


void
RoomContainer::_InitBlitMask()
{
	std::cout << "Initializing blit mask...";
	std::flush(std::cout);

	if (fBlitMask != NULL) {
		fBlitMask->Release();
		fBlitMask = NULL;
	}

	fBlitMask = new Bitmap(AreaRect().w, AreaRect().h, 8);

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
RoomContainer::_InitHeightMap()
{
	std::cout << "Initializing height map...";
	std::flush(std::cout);

	if (fHeightMap != NULL) {
		fHeightMap->Release();
		fHeightMap = NULL;
	}

	std::string heightMapName = fArea->WedName().CString();
	heightMapName += "HT";
	BMPResource* resource = gResManager->GetBMP(heightMapName.c_str());
	if (resource != NULL) {
		fHeightMap = resource->Image();
		gResManager->ReleaseResource(resource);
	}

	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_InitLightMap()
{
	std::cout << "Initializing light map...";
	std::flush(std::cout);

	std::string lightMapName = fArea->WedName().CString();
	lightMapName += "LM";
	BMPResource* resource = gResManager->GetBMP(lightMapName.c_str());
	if (resource != NULL) {
		fLightMap = resource->Image();
		gResManager->ReleaseResource(resource);
	}

	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_InitSearchMap()
{
	std::cout << "Initializing search map...";
	std::flush(std::cout);

	std::string searchMapName = fArea->WedName().CString();
	searchMapName += "SR";
	BMPResource* resource = gResManager->GetBMP(searchMapName.c_str());
	if (resource != NULL) {
		fSearchMap = resource->Image();
		fMapHorizontalRatio = AreaRect().w / fSearchMap->Width();
		fMapVerticalRatio = AreaRect().h / fSearchMap->Height();
		gResManager->ReleaseResource(resource);
	}

	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_DrawBaseMap(GFX::rect mapRect)
{
	MapOverlay *overlay = fOverlays[0];
	if (overlay == NULL) {
		std::cerr << "Overlay 0 is NULL!!" << std::endl;
		return;
	}
	const uint16 overlayWidth = overlay->Width();
	const uint16 firstTileX = fAreaOffset.x / TILE_WIDTH;
	const uint16 firstTileY = fAreaOffset.y / TILE_HEIGHT;
	uint16 lastTileX = firstTileX + (mapRect.w / TILE_WIDTH) + 2;
	uint16 lastTileY = firstTileY + (mapRect.h / TILE_HEIGHT) + 2;

	lastTileX = std::min(lastTileX, overlayWidth);
	lastTileY = std::min(lastTileY, overlay->Height());

	GFX::rect tileRect(0, 0, TILE_WIDTH, TILE_HEIGHT);
	for (uint16 y = firstTileY; y < lastTileY; y++) {
		tileRect.w = TILE_WIDTH;
		tileRect.h = TILE_HEIGHT;
		tileRect.y = y * TILE_HEIGHT - fAreaOffset.y;

		const uint32 tileNumY = y * overlayWidth;
		for (uint16 x = firstTileX; x < lastTileX; x++) {
			tileRect.w = TILE_WIDTH;
			tileRect.x = x * TILE_WIDTH - fAreaOffset.x;

			TileCell* tile = TileAt(tileNumY,  x);
			tile->Draw(fBackBitmap, &tileRect, fDrawOverlays);
		}
	}
}


void
RoomContainer::_DrawAnimations()
{
	if (fAnimations.size() == 0)
		return;

	std::vector<Animation*>::const_iterator i;
	for (i = fAnimations.begin(); i != fAnimations.end(); i++) {
		try {
			Animation* animation = *i;
			if (animation->IsShown()) {
				const Bitmap* frame = animation->NextBitmap();

				DrawObject(frame, animation->Position(), false);
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
RoomContainer::_DrawActors()
{
	std::list<Reference<Object> >::const_iterator a;
	std::list<Reference<Object> > actorsList;
	Core::Get()->GetObjectList(actorsList);
	
	for (a = actorsList.begin();
			a != actorsList.end(); a++) {
		if (Actor* actor = dynamic_cast<Actor*>(a->Target())) {
			try {
				DrawObject(*actor);
			} catch (const char* string) {
				//std::cerr << "_DrawActors: exception: " << string << std::endl;
				continue;
			} catch (...) {
				std::cerr << "Caught exception on actor " << actor->Name() << std::endl;
				continue;
			}
		}
	}
}


void
RoomContainer::_DrawSearchMap(GFX::rect visibleArea)
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
RoomContainer::_UpdateCursor(int x, int y, int scrollByX, int scrollByY)
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
RoomContainer::_ActorAtPoint(const IE::point& point)
{
	// TODO: Maybe there's no need to copy the list in this case
	std::list<Reference<Object> >::const_iterator iter;
	std::list<Reference<Object> > actorList;
	Core::Get()->GetObjectList(actorList);
	
	for (iter = actorList.begin(); iter != actorList.end(); iter++) {
		if (Actor* actor = dynamic_cast<Actor*>(iter->Target())) {
			try {
				const GFX::rect actorFrame = actor->Frame();
				if (rect_contains(actorFrame, point))
					return actor;
			} catch (...) {
				continue;
			}
		}
	}

	return NULL;
}


Region*
RoomContainer::_RegionAtPoint(const IE::point& point)
{
	std::vector<Reference<Region> >::const_iterator i;
	for (i = fRegions.begin(); i != fRegions.end(); i++) {
		IE::rect rect = i->Target()->Frame();
		if (rect_contains(rect_to_gfx_rect(rect), point))
			return i->Target();
	}
	return NULL;
}


Container*
RoomContainer::_ContainerAtPoint(const IE::point& point)
{
	std::vector<Reference<Container> >::const_iterator i;
	for (i = fContainers.begin(); i != fContainers.end(); i++) {
		if (i->Target()->Polygon().Contains(point.x, point.y))
			return i->Target();
	}
	return NULL;
}


Object*
RoomContainer::_ObjectAtPoint(const IE::point& point)
{
	Object* object = NULL;
	const uint16 tileNum = TileNumberForPoint(point);
	if (tileNum >= fTileCells.size())
		return object;

	Door* door = fTileCells[tileNum]->Door();

	if (Actor* actor = _ActorAtPoint(point)) {
		object = actor;
	} else if (door != NULL && rect_contains(door->Frame(), point)) {
		object = door;
	} else if (Container* container = _ContainerAtPoint(point)) {
		object = container;
	} else if (Region* region = _RegionAtPoint(point)) {
		object = region;
		// TODO: This is a side effect of a method
		// which should just return an object, and in
		// fact _ObjectAtPoint() should be const.
		GUI::Get()->SetCursor(region->CursorIndex());
	}

	return object;
}


void
RoomContainer::_LoadOverlays()
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
RoomContainer::_InitTileCells()
{
	std::cout << "Initializing Tile Cells...";
	std::flush(std::cout);
	uint32 numTiles = fOverlays[0]->Size();
	for (uint16 i = 0; i < numTiles; i++) {
		fTileCells.push_back(new TileCell(i, fOverlays, fOverlays.size()));
	}
	std::cout << "Done! Loaded " << numTiles << " tile cells!" << std::endl;
}


void
RoomContainer::_InitVariables()
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
RoomContainer::_InitAnimations()
{
	std::cout << "Initializing Animations...";
	std::flush(std::cout);
	for (uint32 i = 0; i < fArea->CountAnimations(); i++)
		fAnimations.push_back(new Animation(fArea->AnimationAt(i)));
	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_InitRegions()
{
	std::cout << "Initializing Regions...";
	std::flush(std::cout);

	for (uint16 i = 0; i < fArea->CountRegions(); i++) {
		fRegions.push_back(fArea->GetRegionAt(i));
	}
	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_LoadActors()
{
	std::cout << "Loading Actors...";
	std::flush(std::cout);

	// TODO: Check if it's okay
	Party* party = Game::Get()->Party();
	for (uint16 a = 0; a < party->CountActors(); a++) {
		party->ActorAt(a)->SetArea(fArea->Name());
		Core::Get()->RegisterObject(party->ActorAt(a));
	}

	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		Actor* actor = fArea->GetActorAt(i);
		actor->SetArea(fArea->Name());
		Core::Get()->RegisterObject(actor);
	}

	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_InitDoors()
{
	std::cout << "Initializing Doors...";
	std::flush(std::cout);

	assert(fTileCells.size() > 0);

	const uint32 numDoors = fWed->CountDoors();
	for (uint32 c = 0; c < numDoors; c++) {
		Door *door = new Door(fArea->DoorAt(c));
		fWed->GetDoorTiles(door, c);
		for (uint32 i = 0; i < door->fTilesOpen.size(); i++) {
			fTileCells[door->fTilesOpen[i]]->SetDoor(door);
		}
	}
	std::cout << "Done!" << std::endl;
}


void
RoomContainer::_InitContainers()
{
	std::cout << "Initializing Containers...";
	std::flush(std::cout);

	const uint32 numContainers = fArea->CountContainers();
	for (uint32 c = 0; c < numContainers; c++) {
		Container *container = fArea->GetContainerAt(c);
		fContainers.push_back(container);
	}
	std::cout << "Done! Found " << numContainers << " containers!" << std::endl;
}


void
RoomContainer::_UnloadArea()
{
	assert(fWed != NULL);

	if (fSelectedActor != NULL) {
		fSelectedActor.Target()->Select(false);
		fSelectedActor.Unset();
	}

	if (fMouseOverObject != NULL)
	    fMouseOverObject.Unset();
	
	Core::Get()->ExitingArea(this);

	//for (uint32 c = 0; c < fRegions.size(); c++)
		//delete fRegions[c];
	fRegions.clear();

	//for (uint32 c = 0; c < fContainers.size(); c++)
		//delete fContainers[c];
	fContainers.clear();

	for (uint32 c = 0; c < fAnimations.size(); c++)
		delete fAnimations[c];
	fAnimations.clear();

	for (uint32 c = 0; c < fTileCells.size(); c++)
		delete fTileCells[c];
	fTileCells.clear();

	for (uint32 c = 0; c < fOverlays.size(); c++)
		delete fOverlays[c];
	fOverlays.clear();

	gResManager->ReleaseResource(fWed);
	fWed = NULL;
	gResManager->ReleaseResource(fArea);
	fArea = NULL;
	gResManager->ReleaseResource(fBcs);
	fBcs = NULL;
	if (fHeightMap != NULL) {
		fHeightMap->Release();
		fHeightMap = NULL;
	}
	if (fLightMap != NULL) {
		fLightMap->Release();
		fLightMap = NULL;
	}
	if (fSearchMap != NULL) {
		fSearchMap->Release();
		fSearchMap = NULL;
	}

	//gResManager->TryEmptyResourceCache();
}


void
RoomContainer::_UnloadWorldMap()
{
	assert(fWorldMap != NULL);

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


void
RoomContainer::_Unload()
{
	std::cout << "RoomContainer::Unload()" << std::endl;
	if (fWorldMap != NULL)
		_UnloadWorldMap();
	else if (fWed != NULL)
		_UnloadArea();
}
