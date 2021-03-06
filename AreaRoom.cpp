#include "AreaRoom.h"

#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "AreaResource.h"
#include "BackMap.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "BmpResource.h"
#include "Bitmap.h"
#include "Container.h"
#include "Control.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Effect.h"
#include "Game.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Label.h"
#include "Log.h"
#include "MOSResource.h"
#include "Party.h"
#include "Polygon.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"
#include "SearchMap.h"
#include "TextArea.h"
#include "TextSupport.h"
#include "TileCell.h"
#include "TisResource.h"
#include "Timer.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>


static int sSelectedActorRadius = 20;
static int sSelectedActorRadiusStep = 1;


AreaRoom::AreaRoom(const res_ref& areaName, const char* longName,
					const char* entranceName)
	:
	fWed(NULL),
	fArea(NULL),
	fBackMap(NULL),
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
	// Save the entrance name, it will be unloaded in UnloadArea
	std::string savedEntranceName = entranceName ? entranceName : "";

	SetName(areaName.CString());

	GraphicsEngine::Get()->SetWindowCaption(Name());

	std::cout << "Room::Load(" << areaName.CString() << ")" << std::endl;

	fArea = gResManager->GetARA(Name());
	if (fArea == NULL)
		throw std::runtime_error("CANNOT LOAD AREA");

	_InitWed(fArea->WedName().CString());

	GUI* gui = GUI::Get();
	gui->Clear();

	if (!gui->Load("GUIW")) {
		// TODO: Delete other loaded stuff
		gResManager->ReleaseResource(fArea);
		fArea = NULL;
		throw std::runtime_error("CANNOT LOAD GUIW");
	}

	gui->ShowWindow(uint16(-1));
	::Window* window = gui->GetWindow(uint16(-1));

	ShowGUI();

	if (window != NULL) {
		fSavedControl = window->ReplaceControl((uint32)-1, this);
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
	_InitBlitMask();

	IE::point point = { 0, 0 };
	if (!savedEntranceName.empty()) {
		for (uint32 e = 0; e < fArea->CountEntrances(); e++) {
			IE::entrance entrance = fArea->EntranceAt(e);
			//std::cout << "current: " << entrance.name;
			//std::cout << ", looking for " << entranceName << std::endl;

			if (savedEntranceName == entrance.name) {
				point.x = entrance.x;
				point.y = entrance.y;
				SetAreaOffsetCenter(point);
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
		SetAreaOffsetCenter(point);
	}

	Actor* player = Game::Get()->Party()->ActorAt(0);
	if (player != NULL) {
		player->SetPosition(point);
		if (!player->IsSelected())
			player->Select(true);
		fSelectedActor = player;
	}

	GUI::Get()->ShowWindow(999);
	
	::Script* roomScript = Core::Get()->ExtractScript(fArea->ScriptName());

	AddScript(roomScript);
}


AreaRoom::~AreaRoom()
{
	_Unload();
}


WEDResource*
AreaRoom::WED() const
{
	return fWed;
}


ARAResource*
AreaRoom::AREA() const
{
	return fArea;
}


GFX::rect
AreaRoom::AreaRect() const
{
	GFX::rect rect;
	rect.x = rect.y = 0;
	rect.w = fBackMap->Width() * TILE_WIDTH;
	rect.h = fBackMap->Height() * TILE_HEIGHT;

	return rect;
}


::BackMap*
AreaRoom::BackMap() const
{
	return fBackMap;
}


::SearchMap*
AreaRoom::SearchMap() const
{
	return fSearchMap;
}


/* virtual */
void
AreaRoom::Draw()
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

	if (sSelectedActorRadius > 22) {
		sSelectedActorRadiusStep = -1;
	} else if (sSelectedActorRadius < 18) {
		sSelectedActorRadiusStep = 1;
	}
	sSelectedActorRadius += sSelectedActorRadiusStep;

	GFX::rect mapRect = rect_to_gfx_rect(VisibleMapArea());

	bool paused = Core::Get()->IsPaused();
	assert(fBackMap != NULL);
	fBackMap->Update(mapRect, fDrawOverlays);

	if (fDrawAnimations) {
		Timer* timer = Timer::Get("ANIMATIONS");
		bool advance = timer != NULL && timer->Expired() && !paused;
		_DrawAnimations(advance);
	}

	_DrawActors();
	_DrawEffects();

	if (fDrawPolygons) {
		fBackMap->Image()->Lock();
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

					fBackMap->Image()->FillPolygon(*poly, color,
										-AreaOffset().x, -AreaOffset().y);
					fBackMap->Image()->StrokePolygon(*poly, color,
										-AreaOffset().x, -AreaOffset().y);
				}
			}
		}
		fBackMap->Image()->Unlock();
	}

	// TODO: handle this better
	if (Door* door = dynamic_cast<Door*>(fMouseOverObject.Target())) {
		GFX::rect rect = rect_to_gfx_rect(door->Frame());
		rect = offset_rect(rect, -mapRect.x, -mapRect.y);
		fBackMap->Image()->Lock();
		fBackMap->Image()->StrokeRect(rect, 70);
		fBackMap->Image()->Unlock();
	} /*else if (Actor* actor = dynamic_cast<Actor*>(fMouseOverObject.Target())) {
		try {
			GFX::rect rect = rect_to_gfx_rect(actor->Frame());
			rect = offset_rect(rect, -mapRect.x, -mapRect.y);
			fBackMap->Image()->Lock();
			IE::point position = offset_point(actor->Position(), -mapRect.x, -mapRect.y);
			uint32 color = fBackMap->Image()->MapColor(255, 255, 255);
			fBackMap->Image()->StrokeCircle(position.x, position.y, 20, color);
			fBackMap->Image()->Unlock();
		} catch (const char* string) {
			std::cerr << string << std::endl;
		} catch (...) {
		}
	} */else if (Region* region = dynamic_cast<Region*>(fMouseOverObject.Target())) {
		GFX::rect rect = rect_to_gfx_rect(region->Frame());
		rect = offset_rect(rect, -mapRect.x, -mapRect.y);

		uint32 color = 0;
		switch (region->Type()) {
			case IE::REGION_TYPE_TRAVEL:
				color = fBackMap->Image()->MapColor(0, 125, 0);
				break;
			case IE::REGION_TYPE_TRIGGER:
				color = fBackMap->Image()->MapColor(125, 0, 0);
				break;
			default:
				color = fBackMap->Image()->MapColor(255, 255, 255);
				break;
		}

		fBackMap->Image()->Lock();

		if (region->Polygon().CountPoints() > 2) {
			fBackMap->Image()->StrokePolygon(region->Polygon(), color,
								-mapRect.x, -mapRect.y);
		} else
			fBackMap->Image()->StrokeRect(rect, color);
		fBackMap->Image()->Unlock();
	} else if (Container* container = dynamic_cast<Container*>(fMouseOverObject.Target())) {
		uint32 color = 0;
		color = fBackMap->Image()->MapColor(0, 125, 0);
		// TODO: Different colors for trapped/nontrapped
		fBackMap->Image()->Lock();

		if (container->Polygon().CountPoints() > 2) {
			fBackMap->Image()->StrokePolygon(container->Polygon(), color,
										-mapRect.x, -mapRect.y);
		}
		fBackMap->Image()->Unlock();
	}

	GFX::rect screenArea = ViewPort();
	gfx->BlitToScreen(fBackMap->Image(), NULL, &screenArea);
	
	_DrawSearchMap(mapRect);
}


void
AreaRoom::MouseDown(IE::point point)
{
	ConvertFromScreen(point);
	ConvertToArea(point);

	if (fSelectedActor != NULL)
		fSelectedActor.Target()->ClearActionList();

	// TODO: Temporary, for testing
	if (Door* door = dynamic_cast<Door*>(fMouseOverObject.Target())) {
		if (fSelectedActor != NULL)
			fSelectedActor.Target()->ClickedOn(door);
		return;
	} else if (Actor* actor = dynamic_cast<Actor*>(fMouseOverObject.Target())) {
		if (fSelectedActor != actor) {
			//if (fSelectedActor != NULL)
			//	fSelectedActor->Select(false);
			//fSelectedActor = actor;
			//if (fSelectedActor != NULL)
			//	fSelectedActor->Select(true);
			
			if (fSelectedActor != NULL)
				fSelectedActor.Target()->ClickedOn(actor);
		}
		return;
	} else if (Region* region = _RegionAtPoint(point)) {
		// TODO:
		if (fSelectedActor != NULL) {
			fSelectedActor.Target()->ClickedOn(region);
		}
		if (region->Type() == IE::REGION_TYPE_TRAVEL) {
			Core::Get()->LoadArea(region->DestinationArea(), "foo",
					region->DestinationEntrance());
			return;
		} else if (region->Type() == IE::REGION_TYPE_INFO) {
			int32 strRef = region->InfoTextRef();
			std::string text = IDTable::GetDialog(strRef);
			if (strRef >= 0)
				Core::Get()->DisplayMessage(NULL, text.c_str());
			return;
		}
	} else if (Container* container = _ContainerAtPoint(point)) {
		// TODO:
		if (fSelectedActor != NULL) {
			fSelectedActor.Target()->ClickedOn(container);
		}
	}

	if (fSelectedActor != NULL) {
		fSelectedActor.Target()->SetDestination(point);
	}
}


void
AreaRoom::MouseMoved(IE::point point, uint32 transit)
{
	ConvertFromScreen(point);
	ConvertToArea(point);

	if (fWed != NULL) {
		int32 cursor = -1;
		fMouseOverObject = _ObjectAtPoint(point, cursor);
		if (cursor != -1)
			GUI::Get()->SetCursor(cursor);
		else {
			GUI::Get()->SetCursor(IE::CURSOR_WALKTO);
			//GUI::Get()->SetArrowCursor(IE::CURSOR_HAND);
		}
	}
}


void
AreaRoom::DrawBitmap(const Bitmap* bitmap, const IE::point& centerPoint, bool mask)
{
	if (bitmap == NULL)
		return;

	IE::point leftTop = offset_point(centerPoint,
							-(bitmap->Frame().x + bitmap->Frame().w / 2),
							-(bitmap->Frame().y + bitmap->Frame().h / 2));

	GFX::rect rect(leftTop.x, leftTop.y, bitmap->Width(), bitmap->Height());
	if (rects_intersect(rect_to_gfx_rect(VisibleMapArea()), rect)) {
		GFX::rect offsetRect = offset_rect(rect, -AreaOffset().x, -AreaOffset().y);
		if (mask)
			GraphicsEngine::BlitBitmapWithMask(bitmap, NULL,
					fBackMap->Image(), &offsetRect, fBlitMask, &rect);
		else
			GraphicsEngine::BlitBitmap(bitmap, NULL, fBackMap->Image(), &offsetRect);
	}
}


void
AreaRoom::AddAnimation(Animation* animation)
{
	fAnimations.push_back(animation);
}


void
AreaRoom::RemoveAnimation(Animation* animation)
{
	// TODO: Implement
}


void
AreaRoom::AddEffect(Effect* effect)
{
	fEffects.push_back(effect);
}


void
AreaRoom::RemoveEffect(Effect* effect)
{
}


struct TileCompare {
	bool operator() (const TileCell*& lhs, const TileCell*& rhs) const
	{
		return lhs->ID() < rhs->ID();
	}
};


uint32
AreaRoom::GetTileCellsForRegion(std::vector<TileCell*>& cells,
											Region* region)
{
	// TODO: Improve
	std::map<uint16, TileCell* > tileCellsSet;
	IE::rect regionFrame = region->Frame();
	for (int16 x = regionFrame.x_min; x < regionFrame.x_max; x++) {
		for (int16 y = regionFrame.y_min; y < regionFrame.y_max; y++) {
			IE::point point = {x, y};
			TileCell* cell = fBackMap->TileAtPoint(point);
			tileCellsSet[cell->ID()] = cell;
		}
	}
	for (std::map<uint16, TileCell*>::iterator i = tileCellsSet.begin();
			i != tileCellsSet.end(); i++) {
		cells.push_back((*i).second);
	}
	return cells.size();
}


uint8
AreaRoom::PointHeight(const IE::point& point) const
{
	if (fHeightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	return std::min((uint8)fHeightMap->GetPixel(x, y), (uint8)15);
}


uint8
AreaRoom::PointLight(const IE::point& point) const
{
	if (fLightMap == NULL)
		return 8;

	int32 x = point.x / fMapHorizontalRatio;
	int32 y = point.y / fMapVerticalRatio;

	return (uint8)fLightMap->GetPixel(x, y);
}


uint8
AreaRoom::PointSearch(const IE::point& point) const
{
	if (fSearchMap == NULL)
		return 1;

	return 0;
}


/* static */
bool
AreaRoom::IsPointPassable(const IE::point& point)
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room == NULL)
		return true;

	return room->fSearchMap->IsPointPassable(point.x, point.y);
}


void
AreaRoom::ToggleOverlays()
{
	fDrawOverlays = !fDrawOverlays;
}


void
AreaRoom::TogglePolygons()
{
	fDrawPolygons = !fDrawPolygons;
}


void
AreaRoom::ToggleAnimations()
{
	fDrawAnimations = !fDrawAnimations;
}


void
AreaRoom::ToggleSearchMap()
{
	if (++fDrawSearchMap > 2)
		fDrawSearchMap = 0;

	if (fSearchMap == NULL)
		return;

	if (fDrawSearchMap == 2)
		fSearchMap->Image()->SetAlpha(127, true);
	else
		fSearchMap->Image()->SetAlpha(0, true);
}


/* virtual */
void
AreaRoom::ShowGUI()
{
	GUI* gui = GUI::Get();
	gui->ShowWindow(GUI::WINDOW_MESSAGES);
	//gui->ShowWindow(GUI::WINDOW_MESSAGES_LARGE);
	gui->ShowWindow(GUI::WINDOW_COMMANDS);
	gui->ShowWindow(GUI::WINDOW_CMDS);
	gui->ShowWindow(GUI::WINDOW_PLAYER_SLOTS);
}


/* virtual */
void
AreaRoom::HideGUI()
{
	GUI* gui = GUI::Get();
	gui->HideWindow(GUI::WINDOW_MESSAGES);
	//gui->HideWindow(GUI::WINDOW_MESSAGES_LARGE);
	gui->HideWindow(GUI::WINDOW_COMMANDS);
	gui->HideWindow(GUI::WINDOW_CMDS);
	gui->HideWindow(GUI::WINDOW_PLAYER_SLOTS);
}


/* virtual */
bool
AreaRoom::IsGUIShown() const
{
	GUI* gui = GUI::Get();
	return gui->IsWindowShown(GUI::WINDOW_MESSAGES) && gui->IsWindowShown(GUI::WINDOW_COMMANDS);
}


void
AreaRoom::ToggleDayNight()
{
#if 0
	std::string wedName = fWed->Name();
	if (*wedName.rbegin() == 'N')
		wedName = fArea->Name();
	else
		wedName.append("N");
	
	std::vector<std::string> list;
	if (gResManager->GetResourceList(list, wedName.c_str(), RES_WED) > 0)
		_InitWed(wedName.c_str());
#endif
}


/* virtual */
void
AreaRoom::VideoAreaChanged(uint16 width, uint16 height)
{
	GFX::rect rect(0, 0, width, height);
	SetViewPort(rect);
}


void
AreaRoom::_InitBackMap(const GFX::rect& area)
{
	if (fBackMap != NULL)
		delete fBackMap;

	assert(fWed != NULL);
	fBackMap = new ::BackMap(fWed);
}


void
AreaRoom::_InitWed(const char* name)
{
	std::string nightName = name;
	nightName.append("N");

	if (!GameTimer::IsDayTime() && gResManager->ResourceExists(nightName.c_str(), RES_WED))
		fWed = gResManager->GetWED(nightName.c_str());
	else
		fWed = gResManager->GetWED(name);

	_InitBackMap(ViewPort());
	_InitHeightMap();
	_InitLightMap();
	_InitSearchMap();
}


void
AreaRoom::_InitBlitMask()
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
		if (poly == NULL)
			continue;
		uint32 mask = GraphicsEngine::MASK_COMPLETELY;
		if (poly->Flags() & IE::POLY_SHADE_WALL)
			mask = GraphicsEngine::MASK_SHADE;
		if (poly->CountPoints() > 0) {
			fBlitMask->FillPolygon(*poly, mask);
		}
	}
	fBlitMask->Unlock();

	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_InitHeightMap()
{
	std::cout << Log::Normal;
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

	std::cout << Log::Green << "Done!" << std::endl;
}


void
AreaRoom::_InitLightMap()
{
	std::cout << Log::Normal;
	std::cout << "Initializing light map...";
	std::flush(std::cout);

	std::string lightMapName = fArea->WedName().CString();
	lightMapName += "LM";
	BMPResource* resource = gResManager->GetBMP(lightMapName.c_str());
	if (resource != NULL) {
		fLightMap = resource->Image();
		gResManager->ReleaseResource(resource);
	}

	std::cout << Log::Green << "Done!" << std::endl;
}


void
AreaRoom::_InitSearchMap()
{
	std::cout << Log::Normal;
	std::cout << "Initializing search map...";
	std::flush(std::cout);

	std::string searchMapName = fArea->WedName().CString();
	searchMapName += "SR";
	fSearchMap = new ::SearchMap(searchMapName);
	fMapHorizontalRatio = ceilf(float(AreaRect().w) / float(fSearchMap->Width()));
	fMapVerticalRatio = ceilf(float(AreaRect().h) / float(fSearchMap->Height()));
	fSearchMap->SetRatios(fMapHorizontalRatio, fMapVerticalRatio);
	/*std::cout << std::dec;
	std::cout << "map: w=" << AreaRect().w << ", h=";
	std::cout << AreaRect().h << std::endl;
	std::cout << "search map: w=" << fSearchMap->Width() << ", h=";
	std::cout << fSearchMap->Height() << std::endl;
	std::cout << "ratio: h=" << fMapHorizontalRatio;
	std::cout << ", v=" << fMapVerticalRatio << std::endl;	
*/
	std::cout << Log::Green << "Done!" << std::endl;
}


void
AreaRoom::_DrawAnimations(bool advanceFrame)
{
	if (fAnimations.size() == 0)
		return;

	AnimationsList::const_iterator i;
	for (i = fAnimations.begin(); i != fAnimations.end(); i++) {
		try {
			Animation* animation = *i;
			if (animation->IsShown()) {
				if (advanceFrame)
					animation->Next();
				const Bitmap* frame = animation->Bitmap();
				DrawBitmap(frame, animation->Position(), false);
			}
		} catch (std::exception& e) {
			std::cerr << Log::Red << e.what() << std::endl;
			continue;
		}
	}
}


static bool
Finished(Effect* effect)
{
	return effect->Finished();
}


void
AreaRoom::_DrawEffects()
{
	EffectsList::const_iterator i;
	for (i = fEffects.begin(); i != fEffects.end(); i++) {
		try {
			Effect* effect = *i;
			const Bitmap* frame = effect->NextBitmap();
			DrawBitmap(frame, effect->Position(), false);
		} catch (std::exception& e) {
			std::cerr << Log::Red << e.what() << std::endl;
			continue;
		}
	}
	
	// Remove completed effects
	
	EffectsList::iterator it = std::remove_if(fEffects.begin(), fEffects.end(), Finished);
	fEffects.erase(it, fEffects.end());
}


void
AreaRoom::_DrawActors()
{
	ActorsList::const_iterator a;
	ActorsList actorsList;
	Core::Get()->GetActorsList(actorsList);
	std::sort(actorsList.begin(), actorsList.end(), ZOrderSorter());
	
	for (a = actorsList.begin();
			a != actorsList.end(); a++) {
		const Actor* actor = *a;
		try {
			actor->Draw(this, fBackMap->Image());
		} catch (std::exception& ex) {
			std::cerr << Log::Red << ex.what() << std::endl;
			continue;
		}
	}
}


void
AreaRoom::_DrawSearchMap(const GFX::rect& visibleArea)
{
	if ((fSearchMap != NULL && fDrawSearchMap > 0)) {
		GFX::rect destRect(0, ViewPort().h - fSearchMap->Height(),
						fSearchMap->Width(), fSearchMap->Height());
		GraphicsEngine::Get()->BlitToScreen(fSearchMap->Image(), NULL, &destRect);
		GFX::rect scaledRect = visibleArea;
		scaledRect.x /= fMapHorizontalRatio;
		scaledRect.y /= fMapVerticalRatio;
		scaledRect.w /= fMapHorizontalRatio;
		scaledRect.h /= fMapVerticalRatio;
		scaledRect = offset_rect(scaledRect, 0, ViewPort().h - fSearchMap->Height());
		GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(scaledRect, 200);
		
		if (fSelectedActor != NULL) {
			IE::point actorPosition = fSelectedActor.Target()->Position();
			actorPosition.x /= fMapHorizontalRatio;
			actorPosition.y /= fMapVerticalRatio;
			GFX::rect r (actorPosition.x, actorPosition.y, 5, 5 );			
			r = offset_rect(r, 0, ViewPort().h - fSearchMap->Height());
			GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(r, 2000);
		}	
	}
}


Region*
AreaRoom::_RegionAtPoint(const IE::point& point) const
{
	RegionsList::const_iterator i;
	for (i = fRegions.begin(); i != fRegions.end(); i++) {
		IE::rect rect = (*i)->Frame();
		if (rect_contains(rect_to_gfx_rect(rect), point))
			return *i;
	}
	return NULL;
}


Container*
AreaRoom::_ContainerAtPoint(const IE::point& point)
{
	ContainersList::iterator i;
	for (i = fContainers.begin(); i != fContainers.end(); i++) {
		if ((*i)->Polygon().Contains(point.x, point.y))
			return *i;
	}
	return NULL;
}


Object*
AreaRoom::_ObjectAtPoint(const IE::point& point, int32& cursorIndex) const
{
	Object* object = NULL;
	cursorIndex = -1;

	::TileCell* cell = fBackMap->TileAtPoint(point);
	if (cell == NULL)
		return object;

	if (Door* door = cell->Door()) {
		if (rect_contains(door->Frame(), point))
			return door;
	} else {
		std::vector<Actor*> objects;
		if (cell->GetObjects(objects) > 0) {
			std::vector<Actor*>::iterator i;
			for (i = objects.begin(); i != objects.end(); i++) {
				if (rect_contains((*i)->Frame(), point)) {
					object = *i;
					if ((*i)->CRE()->EnemyAlly() < IDTable::EnemyAllyValue("EVILCUTOFF"))
						cursorIndex = IE::CURSOR_TALK;
					else
						cursorIndex = IE::CURSOR_ATTACK;
				}
			}
		}
	}
	
	if (Region* region = _RegionAtPoint(point)) {
		//std::cout << region->Name() << std::endl;
		object = region;
		cursorIndex = region->CursorIndex();
	}

	return object;
}


void
AreaRoom::_InitVariables()
{
	std::cout << "Initializing Variables...";
	std::flush(std::cout);

	uint32 numVars = fArea->CountVariables();
	for (uint32 n = 0; n < numVars; n++) {
		IE::variable var = fArea->VariableAt(n);
		Core::Get()->Vars().Set(var.name, var.value);
	}
	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_InitAnimations()
{
	std::cout << "Initializing Animations...";
	std::flush(std::cout);
	for (uint32 i = 0; i < fArea->CountAnimations(); i++)
		fAnimations.push_back(new Animation(fArea->AnimationAt(i)));
	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_InitRegions()
{
	std::cout << "Initializing Regions...";
	std::flush(std::cout);

	for (uint16 regionIndex = 0; regionIndex < fArea->CountRegions(); regionIndex++) {
		Region* region = fArea->GetRegionAt(regionIndex);
		fRegions.push_back(region);
		Core::Get()->RegisterObject(region);
		std::vector<TileCell*> cells;
		GetTileCellsForRegion(cells, region);
		for (std::vector<TileCell*>::iterator cellIterator = cells.begin();
			cellIterator != cells.end(); cellIterator++) {
			(*cellIterator)->AddRegion(region);
		}

		// TODO: associate room to tile cells
	}
	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_LoadActors()
{
	std::cout << "Loading Actors...";
	std::flush(std::cout);

	// TODO: Check if it's okay
	Party* party = Game::Get()->Party();
	for (uint16 a = 0; a < party->CountActors(); a++) {
		Actor* actor = party->ActorAt(a);
		actor->Acquire();
		actor->SetArea(fArea->Name());
		Core::Get()->RegisterObject(actor);
	}

	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		Actor* actor = fArea->GetActorAt(i);
		actor->SetArea(fArea->Name());
		Core::Get()->RegisterObject(actor);
	}

	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_InitDoors()
{
	std::cout << "Initializing Doors...";
	std::flush(std::cout);

	const uint32 numDoors = fWed->CountDoors();
	for (uint32 c = 0; c < numDoors; c++) {
		Door *door = new Door(fArea->DoorAt(c));
		Core::Get()->RegisterObject(door);
		fWed->LinkDoorWithTiledObject(door);
		for (uint32 i = 0; i < door->fTilesOpen.size(); i++) {
			fBackMap->TileAt(door->fTilesOpen[i])->SetDoor(door);
		}
	}
	std::cout << "Done! Found " << numDoors << " doors." << std::endl;
}


void
AreaRoom::_InitContainers()
{
	std::cout << "Initializing Containers...";
	std::flush(std::cout);

	const uint32 numContainers = fArea->CountContainers();
	for (uint32 c = 0; c < numContainers; c++) {
		Container *container = fArea->GetContainerAt(c);
		fContainers.push_back(container);
		Core::Get()->RegisterObject(container);
	}
	std::cout << "Done! Found " << numContainers << " containers!" << std::endl;
}


void
AreaRoom::_UnloadArea()
{
	assert(fWed != NULL);

	Window()->ReplaceControl(InternalControl()->id, fSavedControl);

	if (fSelectedActor != NULL) {
		fSelectedActor.Target()->Select(false);
		fSelectedActor.Unset();
	}

	if (fMouseOverObject != NULL)
	    fMouseOverObject.Unset();
	
	Core::Get()->ExitingArea(this);

	for (uint32 c = 0; c < fRegions.size(); c++)
		delete fRegions[c];
	fRegions.clear();

	for (uint32 c = 0; c < fContainers.size(); c++)
		delete fContainers[c];
	fContainers.clear();

	for (uint32 c = 0; c < fAnimations.size(); c++)
		delete fAnimations[c];
	fAnimations.clear();

	if (fBackMap != NULL) {
		delete fBackMap;
		fBackMap = NULL;
	}	

	if (fBlitMask != NULL) {
		fBlitMask->Release();
		fBlitMask = NULL;
	}

	gResManager->ReleaseResource(fWed);
	fWed = NULL;
	gResManager->ReleaseResource(fArea);
	fArea = NULL;
	if (fHeightMap != NULL) {
		fHeightMap->Release();
		fHeightMap = NULL;
	}
	if (fLightMap != NULL) {
		fLightMap->Release();
		fLightMap = NULL;
	}
	if (fSearchMap != NULL) {
		delete fSearchMap;
		fSearchMap = NULL;
	}

	gResManager->TryEmptyResourceCache();
}


void
AreaRoom::_Unload()
{
	std::cout << "AreaRoom::Unload()" << std::endl;
	// TODO: On quit, Core has already deleted the GraphicsEngine,
	// so here it's NULL. Change order of object destruction
	// so it doesn't happen.
	GraphicsEngine* gfx = GraphicsEngine::Get();
	if (gfx != NULL)
		gfx->ScreenBitmap()->Clear(0);
	if (fWed != NULL)
		_UnloadArea();
}
