#include "AreaRoom.h"

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
#include "GameTimer.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ITMResource.h"
#include "Label.h"
#include "Log.h"
#include "MOSResource.h"
#include "Party.h"
#include "PathFind.h"
#include "Polygon.h"
#include "Region.h"
#include "ResManager.h"
#include "SearchMap.h"
#include "Script.h"
#include "TextArea.h"
#include "TextSupport.h"
#include "TileCell.h"
#include "TisResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <filesystem>
#include <iostream>
#include <limits.h>
#include <map>
#include <sstream>
#include <stdexcept>


// Where AreaRoom checkpoints an area's own ARE data (see
// ARAResource::WriteToFile()'s own comment) every time it's left -
// relative to the working directory, same convention already used by
// SAVEGAME's own fixed "savegame_slot0.gam" (see scripting/Actions.cpp's
// RunActionSaveGame()), not the BG2 install path - this is this engine's
// own working data, not something to write into a real game installation.
static const char* kAreaCheckpointDir = "SAVEGAME/arecache";


static std::string
_AreaCheckpointPath(const char* areaName)
{
	std::string path = kAreaCheckpointDir;
	path.append("/").append(areaName).append(".ARE");
	return path;
}


/* static */
void
AreaRoom::ClearAreaCheckpoints()
{
	std::error_code error;
	std::filesystem::remove_all(kAreaCheckpointDir, error);
}


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
	SetName(areaName.CString());

	GraphicsEngine::Get()->SetWindowCaption(Name());

	std::cout << "Room::Load(" << areaName.CString() << ")" << std::endl;

	// Revisiting an area left earlier this session: reuse its own
	// ARAResource (kept alive, not released, by _UnloadArea() below)
	// instead of a fresh parse - see Game::AreaCache's own comment for
	// why (door/actor-placement state living directly in that resource
	// would otherwise reset to the file's static defaults every time).
	Game::AreaCache::CachedArea& cache =
		Game::Get()->GetAreaCache()->areas[areaName];
	// Captured before cache.area is (maybe) consumed below - controls
	// which branch _LoadActors() takes (restore cached Actor* objects
	// verbatim, vs. parse fresh ones from fArea) - true only for this
	// in-memory hit, never for the on-disk-checkpoint case just below:
	// a checkpoint file only carries the ARE's own raw actor/door table
	// bytes (see ARAResource::WriteToFile()), not live C++ Actor objects
	// (their own HP/inventory/etc.) - so that case still needs a fresh
	// ARAResource::GetActorAt() parse, just one that now reads back
	// whatever was checkpointed instead of the pristine on-disk default.
	const bool revisited = (cache.area != NULL);
	if (revisited) {
		fArea = cache.area;
		cache.area = NULL;
	} else {
		// No in-memory hit (first load this process run, or after a
		// restart) - try this area's own on-disk checkpoint (written by
		// _UnloadArea() below every time it was previously left, see
		// ARAResource::WriteToFile()'s own comment) before falling back
		// to the pristine KEY/BIF resource.
		ARAResource* checkpoint = new ARAResource(areaName);
		checkpoint->Acquire();
		if (checkpoint->LoadFromFile(_AreaCheckpointPath(Name()).c_str())) {
			fArea = checkpoint;
		} else {
			// ARAResource's destructor is private (Referenceable-managed,
			// like every other Resource) and only ResourceManager is a
			// friend of it - Release() alone (Resource doesn't auto-
			// delete on its own, see Resource : public Referenceable)
			// would just leak it. Safe to route through
			// ReleaseResource() even though this one was never
			// registered via GetResource() - it just won't find
			// anything to also erase from the resource cache.
			gResManager->ReleaseResource(checkpoint);
			fArea = gResManager->GetARA(Name());
		}
	}
	if (fArea == NULL)
		throw std::runtime_error("CANNOT LOAD AREA");

	_InitWed();

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

	Core::Get()->RegisterObject(this);

	_InitVariables();
	_InitAnimations();
	_InitRegions();
	_LoadActors(revisited);
	_InitDoors();
	_InitContainers();
	_InitBlitMask();

	// Loose items dropped on this area's floor on an earlier visit this
	// session (see Game::AreaCache / _UnloadArea()).
	fGroundPiles = std::move(cache.groundPiles);
	cache.groundPiles.clear();

	IE::point point = { 0, 0 };
	IE::entrance entrance;
	if (_GetEntrance(entranceName, entrance)) {
		point.x = entrance.x;
		point.y = entrance.y;
	}

	SetAreaOffsetCenter(point);

	// Every party member needs a real position in this area, not just
	// the lead - a member left at whatever position it had in the
	// *previous* area (only ActorAt(0) was ever repositioned here) can
	// easily land outside this area's own bounds, which
	// GraphicsEngine::BlitBitmapWithMask() doesn't defend against (no
	// clamping against the mask bitmap's own dimensions) - a real,
	// reproduced SEGV in AreaRoom::_DrawActors() right after entering a
	// new area via a script-driven area change (LEAVEAREALUA), where a
	// non-lead party member (e.g. Imoen) still had a stale position from
	// the area just left. Same entrance point for everyone (no formation
	// scatter) - simplest fix that guarantees every member starts inside
	// the new area's bounds; TODO: a small per-member offset would look
	// more like the real engine's spawn formation.
	Party* party = Game::Get()->Party();
	for (uint16 a = 0; a < party->CountActors(); a++) {
		Actor* member = party->ActorAt(a);
		if (member != NULL)
			member->SetPosition(point);
	}

	Actor* player = party->ActorAt(0);
	if (player != NULL)
		SelectActor(player);

	// The HUD (just rebuilt by gui->Load("GUIW") above) needs its
	// portrait bar filled from the party.
	Game::Get()->RefreshHUDPortraits();

	GUI::Get()->ShowWindow(999);

	::Script* roomScript = Core::Get()->ExtractScript(fArea->ScriptName());
	AddScript(roomScript, SCRIPT_LEVEL_DEFAULT);
}


AreaRoom::~AreaRoom()
{
	_UnloadArea();
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


void
AreaRoom::ReloadArea()
{
	if (fWed != NULL) {
		gResManager->ReleaseResource(fWed);
		fWed = NULL;
	}

	_InitWed();
}


GFX::rect
AreaRoom::AreaRect() const
{
	// Complete map rect, starting at 0, 0
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
AreaRoom::Update(bool runScripts)
{
	Object::Update(runScripts);

	// TODO: other objects
	ActorsList::iterator i;
	for (auto& actor : fActors) {
		actor->Update(runScripts);
	}

	_CleanDestroyedObjects();
}


/* virtual */
void
AreaRoom::Draw()
{
	GraphicsEngine* gfx = GraphicsEngine::Get();

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
	_DrawGroundPiles();
	_DrawEffects();

	if (fDrawPolygons)
		_DrawPolygons(mapRect);

	if (fMouseOverObject.Target() != NULL) {
		::Outline outline = fMouseOverObject.Target()->Outline();
		GFX::Color rgbColor = outline.Color();
		uint32 color = fBackMap->Image()->MapRGBColor(rgbColor.r, rgbColor.g, rgbColor.b);
		fBackMap->Image()->Lock();
		if (outline.Type() == Outline::OUTLINE_RECT) {
			GFX::rect rect = rect_to_gfx_rect(outline.Rect());
			ConvertFromArea(rect);
			fBackMap->Image()->StrokeRect(rect, color);
		} else {
			::Polygon polygon = outline.Polygon();
			fBackMap->Image()->StrokePolygon(polygon, color,
											-mapRect.x, -mapRect.y);
		}
		fBackMap->Image()->Unlock();
	}

	if (fSelectedActor.Target() != NULL && fSelectedActor.Target()->IsWalking()) {
		IE::point mapOffset = { mapRect.x, mapRect.y };
		IE::point destination = fSelectedActor.Target()->Destination() - mapOffset;

		fBackMap->Image()->Lock();
		uint32 color = fBackMap->Image()->MapRGBColor(0, 255, 0);
		fBackMap->Image()->StrokeCircle(destination.x, destination.y, 10, color);
		fBackMap->Image()->Unlock();
	}

	GFX::rect screenArea = Control::Frame();
	gfx->BlitToScreen(fBackMap->Image(), NULL, &screenArea);

#if 1
	// Show mouse position as string
	const Font* font = FontRoster::GetFont("TOOLFONT");
	IE::point mousePosition;
	GUI::Get()->GetCursorPosition(mousePosition.x, mousePosition.y);
	IE::point areaPosition = mousePosition;
	ConvertToArea(areaPosition);
	std::ostringstream text;
	text << areaPosition.x << ", " << areaPosition.y;
	::Bitmap* bitmap = font->GetRenderedString(text.str().c_str(), 0);
	GFX::rect rect = bitmap->Frame();
	rect.x = mousePosition.x;
	rect.y = mousePosition.y - 10;
	gfx->BlitToScreen(bitmap, NULL, &rect);
	bitmap->Release();
#endif

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
	} else if (Region* region = RegionAtPoint(point)) {
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
				Core::Get()->DisplayMessage(region, text.c_str());
			return;
		}
	} else if (Container* container = _ContainerAtPoint(point)) {
		// TODO:
		if (fSelectedActor != NULL) {
			fSelectedActor.Target()->ClickedOn(container);
		}
	}

	// A loose pile dropped on the floor: the selected party member loots
	// it. No walk-to-pile yet (declared simplification) - pickup is
	// immediate on click.
	int32 pileIndex = _GroundPileAtPoint(point);
	if (pileIndex >= 0) {
		if (fSelectedActor != NULL)
			PickUpGroundPile((size_t)pileIndex, fSelectedActor.Target());
		return;
	}

	if (fSelectedActor != NULL) {
		action_params* params = new action_params;
		strcpy(params->Second()->name, fSelectedActor.Target()->Name());
		params->where = point;
		params->id = 23; // MOVETOPOINT
		fSelectedActor.Target()->AddAction(params);
		params->Release();
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
AreaRoom::SelectActor(Actor* actor)
{
	if (fSelectedActor.Target() == actor)
		return;

	if (fSelectedActor != NULL)
		fSelectedActor.Target()->Select(false);

	fSelectedActor = actor;

	if (actor != NULL)
		actor->Select(true);
}


Actor*
AreaRoom::SelectedActor() const
{
	return fSelectedActor.Target();
}


// Takes into account the view offset
void
AreaRoom::DrawBitmap(const Bitmap* bitmap, const IE::point& centerPoint, bool mask)
{
	if (bitmap == NULL)
		return;

	IE::point leftTop = offset_point(centerPoint,
							-(bitmap->Frame().x + bitmap->Frame().w / 2),
							-(bitmap->Frame().y + bitmap->Frame().h / 2));

	GFX::rect rect(leftTop.x, leftTop.y, bitmap->Width(), bitmap->Height());
	GFX::rect areaRect = rect_to_gfx_rect(VisibleMapArea());
	if (rect.Intersects(areaRect)) {
		GFX::rect offsetRect = rect;
		ConvertFromArea(offsetRect);
		if (mask)
			GraphicsEngine::BlitBitmapWithMask(bitmap, NULL,
					fBackMap->Image(), &offsetRect, fBlitMask, &rect);
		else
			GraphicsEngine::BlitBitmap(bitmap, NULL, fBackMap->Image(), &offsetRect);
	}
}


void
AreaRoom::AddObject(Object* object)
{
	object->SetArea(this);
	Core::Get()->RegisterObject(object);

	// TODO: Other objects
	switch (object->Type()) {
		case Object::ACTOR:
			fActors.push_back(dynamic_cast<Actor*>(object));
			break;
		case Object::REGION:
			fRegions.push_back(dynamic_cast<Region*>(object));
			break;
		case Object::CONTAINER:
			fContainers.push_back(dynamic_cast<Container*>(object));
			break;
		case Object::DOOR:
			fDoors.push_back(dynamic_cast<Door*>(object));
			break;
		default:
			break;
	}
}


void
AreaRoom::RemoveObject(Object* object)
{
	// Only the case MoveBetweenAreasEffect (see scripting/Actions.cpp)
	// actually needs: pulling a live Actor out of fActors before handing
	// it to Game::TempState for another AreaRoom to pick up, so it isn't
	// also released a second time by this room's own _UnloadArea() (that
	// unconditionally Release()s everything still in fActors when this
	// room goes away).
	switch (object->Type()) {
		case Object::ACTOR:
		{
			Actor* actor = dynamic_cast<Actor*>(object);
			auto pos = std::find(fActors.begin(), fActors.end(), actor);
			if (pos != fActors.end())
				fActors.erase(pos);
			break;
		}
		default:
			// TODO: Other objects
			break;
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


int32
AreaRoom::GetActorsList(ActorsList& list) const
{
	list = fActors;
	return list.size();
}


int32
AreaRoom::ActorsCount() const
{
	return fActors.size();
}


Actor*
AreaRoom::ActorAt(int32 index) const
{
	return fActors[index];
}


void
AreaRoom::RemoveEffect(Effect* effect)
{
}


Object*
AreaRoom::GetObject(const char* name) const
{
	// TODO: containers, doors, other objects
	for (const auto &actor : fActors) {
		if (!strcasecmp(name, actor->Name()))
			return actor;
	}

	for (const auto& region : fRegions) {
		if (!strcasecmp(name, region->Name()))
			return region;
	}

	for (const auto& door : fDoors) {
		if (!strcasecmp(name, door->Name()))
			return door;
	}

	for (const auto& container :  fContainers) {
		if (!strcasecmp(name, container->Name()))
			return container;
	}
	return NULL;
}


Object*
AreaRoom::GetObject(uint16 globalEnum) const
{
	// TODO: containers, doors, other objects
	for (const auto& object : fActors) {
		if (object != NULL && object->GlobalID() == globalEnum)
			return object;
	}

	return NULL;
}


Actor*
AreaRoom::GetObjectFromNode(object_params* node) const
{
	// TODO: Simplify, merge code.

	for (const auto& actor : fActors) {
		if (actor->MatchNode(node)) {
			//std::cout << "returned " << (*i)->Name() << std::endl;
			//(*i)->Print();
			return actor;
		}
	}

	return NULL;
}


Actor*
AreaRoom::GetObject(const Region* region) const
{
	// TODO: Only returns the first object!
	// TODO: containers, doors, other objects
	for (const auto &actor : fActors) {
		if (region->Contains(actor->Position()))
			return actor;
	}

	return NULL;
}


Actor*
AreaRoom::GetNearestEnemyOf(const Actor* object) const
{
	int minDistance = INT_MAX;
	Actor* nearest = NULL;
	for (const auto& actor : fActors) {
		if (actor != object && actor->IsEnemyOf(object)) {
			int distance = Distance(object, actor);
			if (distance < minDistance) {
				minDistance = distance;
				nearest = actor;
			}
		}
	}

	return nearest;
}


Actor*
AreaRoom::GetNearestEnemyOfType(const Actor* object, int ieClass) const
{
	int minDistance = INT_MAX;
	Actor* nearest = NULL;
	for (const auto& actor : fActors) {
		if (actor != object && actor->IsEnemyOf(object) && actor->IsClass(ieClass)) {
			int distance = Distance(object, actor);
			if (distance < minDistance) {
				minDistance = distance;
				nearest = actor;
			}
		}
	}
	if (nearest != NULL) {
		std::cout << "Nearest Enemy of " << object->Name();
		std::cout << " (type " << ieClass << ")";
		std::cout << " is " << nearest->Name() << std::endl;
	}

	return nearest;
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
	for (auto tileCellSet : tileCellsSet) {
		cells.push_back(tileCellSet.second);
	}
	return cells.size();
}


int
AreaRoom::Distance(const Object* a, const Object* b) const
{
	const Actor* actor = dynamic_cast<const Actor*>(a);
	if (actor == NULL) {
		std::cerr << "Distance: requested for non-actor object!" << std::endl;
		return 0;
	}

	const IE::point positionA = actor->Position();
	const IE::point positionB = b->NearestPoint(positionA);

	IE::point invalidPoint = { -1, -1 };
	if (positionA == invalidPoint && positionB == invalidPoint)
		return 100; // TODO: ???

	return point_distance(positionA, positionB);
}


const std::vector<Door*>&
AreaRoom::Doors() const
{
	return fDoors;
}


const std::vector<Container*>&
AreaRoom::Containers() const
{
	return fContainers;
}


// Icons of ground piles are small; a pile within this many pixels of a
// drop point (or click) counts as "the same spot".
static const int kGroundPileMergeRadius = 24;


void
AreaRoom::AddGroundItem(const IE::item& item, const IE::point& position)
{
	for (auto& pile : fGroundPiles) {
		if (std::abs(pile.position.x - position.x) <= kGroundPileMergeRadius
			&& std::abs(pile.position.y - position.y) <= kGroundPileMergeRadius) {
			pile.items.push_back(item);
			return;
		}
	}
	IE::ground_pile pile;
	pile.position = position;
	pile.items.push_back(item);
	fGroundPiles.push_back(std::move(pile));
}


const std::vector<IE::ground_pile>&
AreaRoom::GroundPiles() const
{
	return fGroundPiles;
}


void
AreaRoom::SetGroundPiles(std::vector<IE::ground_pile> piles)
{
	fGroundPiles = std::move(piles);
}


void
AreaRoom::PickUpGroundPile(size_t index, Actor* taker)
{
	if (index >= fGroundPiles.size() || taker == NULL || taker->CRE() == NULL)
		return;

	std::vector<IE::item>& items = fGroundPiles[index].items;
	for (auto it = items.begin(); it != items.end(); ) {
		if (taker->AddItem(it->name, it->quantity1 > 0 ? it->quantity1 : 1))
			it = items.erase(it);
		else
			++it; // no room - leave it on the ground
	}

	if (items.empty())
		fGroundPiles.erase(fGroundPiles.begin() + index);
}


int32
AreaRoom::_GroundPileAtPoint(const IE::point& areaPoint) const
{
	for (size_t i = 0; i < fGroundPiles.size(); i++) {
		const IE::point& p = fGroundPiles[i].position;
		if (std::abs(p.x - areaPoint.x) <= kGroundPileMergeRadius
			&& std::abs(p.y - areaPoint.y) <= kGroundPileMergeRadius)
			return (int32)i;
	}
	return -1;
}


void
AreaRoom::_DrawGroundPiles()
{
	if (fGroundPiles.empty())
		return;

	for (const auto& pile : fGroundPiles) {
		if (pile.items.empty())
			continue;
		// Draw the top item's ground icon (its inventory icon as a
		// fallback - some items have no dedicated ground graphic).
		ITMResource* itm = gResManager->GetITM(pile.items.back().name);
		if (itm == NULL)
			continue;
		res_ref iconRef = itm->GroundIcon();
		if (iconRef.name[0] == '\0')
			iconRef = itm->InventoryIcon();
		BAMResource* bam = gResManager->GetBAM(iconRef);
		if (bam != NULL) {
			Bitmap* frame = bam->FrameForCycle(0, 0);
			if (frame != NULL) {
				DrawBitmap(frame, pile.position, false);
				frame->Release();
			}
			gResManager->ReleaseResource(bam);
		}
		gResManager->ReleaseResource(itm);
	}
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


/* static */
bool
AreaRoom::PointDoesNotBlockLight(const IE::point& point)
{
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (room == NULL)
		return true;

	return !room->fSearchMap->BlocksLight(point.x, point.y);
}


bool
AreaRoom::HasLineOfSight(const IE::point& from, const IE::point& to) const
{
	// PathFinder::_WalkLine() (what HasLineOfSight() itself calls) steps
	// pixel by pixel regardless of the constructor's step argument - it's
	// only relevant to GeneratePath()'s own A* search, unused here.
	PathFinder finder(PathFinder::kStep, PointDoesNotBlockLight);
	return finder.HasLineOfSight(from, to);
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

	if (fDrawSearchMap == 1)
		fSearchMap->Image()->SetAlpha(255, false);
	else if (fDrawSearchMap == 2)
		fSearchMap->Image()->SetAlpha(127, true);
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
	gui->HideWindow(GUI::WINDOW_MESSAGES_LARGE);
	gui->HideWindow(GUI::WINDOW_COMMANDS);
	gui->HideWindow(GUI::WINDOW_CMDS);
	gui->HideWindow(GUI::WINDOW_PLAYER_SLOTS);
}


/* virtual */
bool
AreaRoom::IsGUIShown() const
{
	GUI* gui = GUI::Get();
	return (gui->IsWindowShown(GUI::WINDOW_MESSAGES) || gui->IsWindowShown(GUI::WINDOW_MESSAGES))
			&& gui->IsWindowShown(GUI::WINDOW_COMMANDS);
}


bool
AreaRoom::CanRest() const
{
	return fCanRest;
}


void
AreaRoom::SetCanRest(bool canRest)
{
	fCanRest = canRest;
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
AreaRoom::_InitWed()
{
	assert(fWed == NULL);

	std::string nightName = fArea->WedName().CString();
	nightName.append("N");

	if (!GameTimer::IsDayTime() && gResManager->ResourceExists(nightName.c_str(), RES_WED))
		fWed = gResManager->GetWED(nightName.c_str());
	else
		fWed = gResManager->GetWED(fArea->WedName().CString());

	_InitBackMap(Control::Frame());
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

	std::cout << Log::Green << "Done!" << Log::Normal << std::endl;
}


void
AreaRoom::_DrawAnimations(bool advanceFrame)
{
	for (const auto& animation : fAnimations) {
		try {
			if (animation->IsShown()) {
				const Bitmap* frame = animation->Bitmap();
				if (advanceFrame)
					animation->NextFrame();
				DrawBitmap(frame, animation->Position(), false);
			}
		} catch (std::exception& e) {
			std::cerr << Log::Red << e.what() << Log::Normal << std::endl;
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
			const Bitmap* frame = effect->Bitmap();
			DrawBitmap(frame, effect->Position(), false);
			if (!Core::Get()->IsPaused()) {
				effect->AdvanceFrame();
			}
		} catch (std::exception& e) {
			std::cerr << Log::Red << e.what() << Log::Normal << std::endl;
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
	ActorsList visibleActors;
	for (auto actor : fActors) {
		if (_IsVisibleOnScreen(actor))
			visibleActors.push_back(actor);
	}

	// Sort visible actors by z-order
	std::sort(visibleActors.begin(), visibleActors.end(), ZOrderSorter());
	for (auto visibleActor : visibleActors) {
		try {
			visibleActor->Draw(this);
		} catch (std::exception& ex) {
			// TODO: too much spam
			//std::cerr << Log::Red << ex.what() << Log::Normal << std::endl;
			continue;
		}
	}
}


void
AreaRoom::_DrawPolygons(const GFX::rect& mapRect)
{
	fBackMap->Image()->Lock();
	for (uint32 p = 0; p < fWed->CountPolygons(); p++) {
		const Polygon* poly = fWed->PolygonAt(p);
		if (poly != NULL && poly->CountPoints() > 0) {
			if (poly->Frame().Intersects(mapRect)) {
				uint32 color = 0;
				if (poly->Flags() & IE::POLY_SHADE_WALL)
					color = 200;
				else if (poly->Flags() & IE::POLY_HOVERING)
					color = 500;
				else if (poly->Flags() & IE::POLY_COVER_ANIMATIONS)
					color = 1000;

				fBackMap->Image()->FillPolygon(*poly, color, -AreaOffset().x,
												-AreaOffset().y);
				fBackMap->Image()->StrokePolygon(*poly, color, -AreaOffset().x,
													-AreaOffset().y);
			}
		}
	}
	fBackMap->Image()->Unlock();
}


void
AreaRoom::_DrawSearchMap(const GFX::rect& visibleArea)
{
	if (fSearchMap != NULL && fDrawSearchMap > 0) {
		GFX::rect viewPort = Control::Frame();
		GFX::point destPoint = {80, int16(viewPort.h - fSearchMap->Height() - 80)};
		GraphicsEngine::Get()->BlitToScreen(fSearchMap->Image(), destPoint);
		GFX::rect scaledRect = visibleArea;
		scaledRect.x /= fMapHorizontalRatio;
		scaledRect.y /= fMapVerticalRatio;
		scaledRect.w /= fMapHorizontalRatio;
		scaledRect.h /= fMapVerticalRatio;
		scaledRect.OffsetBy(destPoint.x, destPoint.y);
		GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(scaledRect, 200);

		if (fSelectedActor != NULL) {
			IE::point actorPosition = fSelectedActor.Target()->Position();
			actorPosition.x /= fMapHorizontalRatio;
			actorPosition.y /= fMapVerticalRatio;
			GFX::rect r (actorPosition.x, actorPosition.y, 5, 5 );
			r.OffsetBy(destPoint.x, destPoint.y);
			GraphicsEngine::Get()->ScreenBitmap()->StrokeRect(r, 2000);
		}
	}
}


Region*
AreaRoom::RegionAtPoint(const IE::point& point) const
{
	for (auto region : fRegions) {
		GFX::rect rect = rect_to_gfx_rect(region->Frame());
		if (rect.Contains(point.x, point.y))
			return region;
	}
	return NULL;
}


void
AreaRoom::ClearAllActions(Object* keep)
{
	// The Infinity Engine's ClearAllActions() clears every *other* actor's
	// queue, never the caller's own. Cutscene blocks rely on this: a beat
	// routinely queues ClearAllActions() onto its driving actor *ahead of*
	// that same actor's remaining actions (BG1's Candlekeep departure,
	// CH1CUT01, queues MoveToPoint -> ClearAllActions -> DayNight ->
	// LeaveAreaLUA all onto Player1). Clearing the caller too would drop
	// the LeaveAreaLUA and strand the protagonist in the old area.
	for (auto actor : fActors) {
		if (actor != keep)
			actor->ClearActionList();
	}
}


Container*
AreaRoom::_ContainerAtPoint(const IE::point& point) const
{
	for (const auto container : fContainers) {
		if (container->Polygon().Contains(point.x, point.y))
			return container;
	}
	return NULL;
}


Actor*
AreaRoom::_ActorAtPoint(const IE::point& point) const
{
	for (const auto actor : fActors) {
		if (rect_contains(actor->Frame(), point))
			return actor;
	}
	return nullptr;
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
	} else if (Actor *actor = _ActorAtPoint(point)) {
		object = actor;
		if (actor->CRE()->EnemyAlly() < IDTable::EnemyAllyValue("EVILCUTOFF"))
			cursorIndex = IE::CURSOR_TALK;
		else
			cursorIndex = IE::CURSOR_ATTACK;
	}

	if (Region* region = RegionAtPoint(point)) {
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
		AddObject(region);
		std::vector<TileCell*> cells;
		GetTileCellsForRegion(cells, region);
		for (auto cell : cells) {
			cell->AddRegion(region);
		}

		// TODO: associate room to tile cells
	}
	std::cout << "Done!" << std::endl;
}


void
AreaRoom::_LoadActors(bool revisited)
{
	std::cout << "AreaRoom: Loading Actors..." << std::endl;

	// TODO: Check if it's okay
	std::cout << "- Loading party actors:" ;
	std::flush(std::cout);
	Party* party = Game::Get()->Party();
	for (uint16 a = 0; a < party->CountActors(); a++) {
		Actor* actor = party->ActorAt(a);
		actor->Acquire();
		AddObject(actor);
		std::cout << " + ";
		std::flush(std::cout);
	}
	std::cout << Log::Normal << std::endl;

	Game::TempState* tempState = Game::Get()->GetTempState();
	if (tempState != NULL) {
		auto pending = tempState->actors.find(Name());
		if (pending != tempState->actors.end()) {
			std::cout << "- Loading actors from previous room:" ;
			std::cout << std::endl;
			for (const Game::TempState::PendingActor& entry : pending->second) {
				Actor* actor = entry.actor;
				AddObject(actor);
				actor->SetPosition(entry.position);
				actor->SetOrientation(entry.orientation);
				actor->Release();
				std::cout << "\t + ";
				std::cout << actor->LongName() << "(" << actor->Name() << ")";
				std::cout << "(id: " << actor->GlobalID() << ")";
				std::cout << std::endl;
				std::flush(std::cout);
			}
			tempState->actors.erase(pending);
			std::cout << std::endl;
		}
	}

	std::cout << "- Loading other actors:" ;
	std::cout << std::endl;
	if (revisited) {
		// Restore exactly who was left here, with their own carried-over
		// state (position, HP, inventory, NumTimesTalkedTo, ...) - even
		// an empty list is meaningful (nobody survived last time), so
		// this must not fall through to the fresh-parse branch below,
		// which would wrongly resurrect the area's originally-placed
		// actors. See Game::AreaCache's own comment.
		Game::AreaCache::CachedArea& cache =
			Game::Get()->GetAreaCache()->areas[Name()];
		for (Actor* actor : cache.actors) {
			AddObject(actor);
			// No Release() here (unlike the TempState loop above) - the
			// single reference _UnloadArea() Acquire()'d for the cache is
			// exactly the one fActors needs now; there was never a second,
			// temporary one to give back (TempState's extra Acquire() has
			// to coexist with the source area's own still-in-fActors
			// reference for a moment, since RemoveObject() deliberately
			// doesn't release that one - not the case here, where the
			// cache Acquire() and the old fActors membership's Release()
			// happen back to back in the very same _UnloadArea() loop).
			std::cout << "\t + ";
			std::cout << actor->LongName() << "(" << actor->Name() << ")";
			std::cout << "(id: " << actor->GlobalID() << ")";
			std::cout << std::endl;
			std::flush(std::cout);
		}
		cache.actors.clear();
	} else {
		for (uint16 i = 0; i < fArea->CountActors(); i++) {
			Actor* actor = fArea->GetActorAt(i);
			AddObject(actor);
			std::cout << "\t + ";
			std::cout << actor->LongName() << "(" << actor->Name() << ")";
			std::cout << "(id: " << actor->GlobalID() << ")";
			std::cout << std::endl;
			std::flush(std::cout);
		}
	}
	std::cout << std::endl;

	std::cout << Log::Green << "AreaRoom: done loading actors!" << Log::Normal << std::endl;
}


void
AreaRoom::_InitDoors()
{
	std::cout << "Initializing Doors...";
	std::flush(std::cout);

	const uint32 numDoors = fWed->CountDoors();
	for (uint32 c = 0; c < numDoors; c++) {
		// The door count comes from the WED; the door data from the ARE.
		// A malformed area (e.g. AR0900) can list more doors in the WED
		// than the ARE actually carries - DoorAt() returns NULL for those.
		IE::door* areaDoor = fArea->DoorAt(c);
		if (areaDoor == NULL)
			continue;
		Door *door = new Door(areaDoor, fArea);
		AddObject(door);
		door->UpdateSearchMapBlocking();
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

	const auto& cachedContents =
		Game::Get()->GetAreaCache()->areas[Name()].containerContents;

	const uint32 numContainers = fArea->CountContainers();
	for (uint32 c = 0; c < numContainers; c++) {
		Container *container = fArea->GetContainerAt(c);
		// Visited this area earlier this session: restore what was left
		// in this container (looting isn't written back to the ARE
		// resource, so GetContainerAt() just re-parsed its original
		// contents).
		auto cached = cachedContents.find(c);
		if (cached != cachedContents.end())
			container->SetContainerItems(cached->second);
		AddObject(container);
	}
	std::cout << "Done! Found " << numContainers << " containers!" << std::endl;
}


// An actor's current region (Actor::fRegion) is only cleared by its own
// movement, via Actor::_UpdateRegions() calling Region::ActorExited() when
// the actor's position leaves the region's bounds - there's no equivalent
// hook for an actor being destroyed while still standing inside one.
// Without this, the region's fObjectsInside (a raw, unreferenced Actor*
// list) keeps a dangling pointer to the destroyed actor, which a later
// Region::ActorEntered()/IsActorInside() call (from some other, still-alive
// actor walking through) dereferences - a real heap-use-after-free, found
// via a crash in the BG2 opening cutscene once _CleanDestroyedObjects()
// below actually started freeing actors (see its own comment).
//
// Region::ActorExited() only detaches this actor from *the region's own*
// list - it doesn't touch the actor's own fRegion, so also clear that
// here (SetRegion(NULL), same as what _UpdateRegions() itself does right
// after its own ActorExited() call). Without it, an actor that survives
// its area unloading (a party member, or now a Game::AreaCache-cached
// one - see its own comment) keeps pointing at a Region object that's
// about to be destroyed a few lines below in _UnloadArea() - a second,
// separate heap-use-after-free, found the same way as the first: an
// actor cached and restored into AR0602 across a real area round-trip
// (never having moved again, so Actor::_UpdateRegions() never got a
// chance to self-correct fRegion) crashed on the area's *next* unload.
static void
_DetachFromCurrentRegion(Actor* actor)
{
	Region* region = actor->CurrentRegion();
	if (region != NULL) {
		region->ActorExited(actor);
		actor->SetRegion(NULL);
	}
}


void
AreaRoom::_CleanDestroyedObjects()
{
	// TODO: Remove other objects?
	ActorsList::iterator i = fActors.begin();
	while (i != fActors.end()) {
		Actor* actor = *i;
		if (actor->ToBeDestroyed()) {
			if (actor == Core::Get()->CutsceneActor()) {
				// TODO: is this correct ?
				// Clear the dangling reference before releasing the
				// actor below - same convention Core::UpdateLogic()
				// already follows at its own EndCutsceneMode() call
				// site (nils fCutsceneActor first).
				Core::Get()->SetCutsceneActor(NULL);
				Core::Get()->EndCutsceneMode();
				//return;
			}
			std::cout << "Destroy actor " << actor->Name() << std::endl;
			_DetachFromCurrentRegion(actor);
			actor->ClearActionList();
			actor->SetArea(NULL);
			Core::Get()->UnregisterObject(actor);

			i = fActors.erase(i);
			// Mirror _UnloadArea()'s cleanup: fActors held an implicit
			// reference (from construction for a plain actor, or from
			// the extra Acquire() _LoadActors() takes for party members)
			// that must be released now that the actor is no longer in
			// the list
			actor->Release();
		} else
			i++;
	}
}


void
AreaRoom::_UnloadArea()
{
	std::cout << "AreaRoom::_UnloadArea(" << Name() << ")" << std::endl;
	// TODO: On quit, Core has already deleted the GraphicsEngine,
	// so here it's NULL. Change order of object destruction
	// so it doesn't happen.
	GraphicsEngine* gfx = GraphicsEngine::Get();
	if (gfx != NULL)
		gfx->ScreenBitmap()->Clear(0);

	if (fSavedControl != nullptr) {
		if (Window() != nullptr)
			Window()->ReplaceControl(InternalControl()->id, fSavedControl);
		else
			delete fSavedControl;
		fSavedControl = nullptr;
	}

	SelectActor(NULL);

	if (fMouseOverObject != NULL)
		fMouseOverObject.Unset();

	ClearScripts();

	for (auto actor : fActors) {
		//UnregisterObject(*i);
		// TODO: NOT CORRECT, but if an object has actions, they keep a reference to the object
		// and this blocks deletion of said object
		if (actor == Core::Get()->CutsceneActor()) {
			// Same dangling-reference guard as _CleanDestroyedObjects()
			// above (see its own comment) - reachable here too now that
			// an area change mid-cutscene (see Core::RequestAreaChange())
			// can unload this area, and thus release this actor, while
			// it's still Core's fCutsceneActor.
			Core::Get()->SetCutsceneActor(NULL);
			Core::Get()->EndCutsceneMode();
		}
		// A party member survives this - Release() below only drops the
		// implicit reference _LoadActors() gave this room, Party still
		// owns it - and keeps going in the new area, so clearing its
		// action list here would throw away a legitimate in-progress
		// action instead of merely one belonging to an actor that's
		// actually going away. Reproduced: a cutscene queuing a
		// multi-tick FADEFROMCOLOR on the party leader to fade back in
		// once the new area loads - ClearActionList() wiped it out
		// mid-fade (right after its first call, which sets the fade to
		// fully black), leaving the screen stuck black forever since
		// nothing else ever finishes raising it back up.
		if (!actor->InParty()) {
			actor->ClearActionList();
			// Cache it (see Game::AreaCache's own comment) instead of
			// just letting Release() below drop it to 0 and destroy it -
			// this area's own actors (an NPC's dialogue state like
			// NumTimesTalkedTo, quest globals tied to them, etc.) should
			// still be there if the party comes back. Unlike a surviving
			// party member (always immediately re-added to whichever
			// area loads next, so its own stale Area() pointer here is
			// never actually read), a cached actor can sit parked for a
			// long time before this specific area is revisited (if
			// ever) - nil its Area() so nothing reads a dangling
			// AreaRoom* in the meantime, same as _CleanDestroyedObjects()
			// already does for actors that are actually being destroyed.
			actor->Acquire();
			actor->SetArea(NULL);
			Game::Get()->GetAreaCache()->areas[Name()].actors.push_back(actor);
		}
		_DetachFromCurrentRegion(actor);

		actor->Release();
	}
	fActors.clear();

	Core::Get()->ExitingArea(this);

	for (uint32 c = 0; c < fRegions.size(); c++) {
		if (fRegions[c] != NULL)
			fRegions[c]->Release();
	}
	fRegions.clear();

	// Remember each container's remaining contents (looting mutates the
	// C++ Container, never the ARE resource) so a chest emptied this
	// session stays empty on re-entry - see _InitContainers().
	auto& cachedContents =
		Game::Get()->GetAreaCache()->areas[Name()].containerContents;
	for (uint32 c = 0; c < fContainers.size(); c++) {
		if (fContainers[c] != NULL) {
			cachedContents[c] = fContainers[c]->ContainerItems();
			fContainers[c]->Release();
		}
	}
	fContainers.clear();

	for (uint32 c = 0; c < fDoors.size(); c++) {
		if (fDoors[c] != NULL)
			fDoors[c]->Release();
	}
	fDoors.clear();

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
	// Checkpoint this area's own current state to disk too (not just the
	// in-memory cache below) - see ARAResource::WriteToFile()'s own
	// comment for exactly what that captures. create_directories() is
	// idempotent/cheap - simpler to call every time than to track
	// whether it's already been done this run.
	std::error_code checkpointError;
	std::filesystem::create_directories(kAreaCheckpointDir, checkpointError);
	if (!fArea->WriteToFile(_AreaCheckpointPath(Name()).c_str())) {
		std::cerr << "AreaRoom::_UnloadArea(): failed to checkpoint "
			<< Name() << std::endl;
	}

	// Kept alive in the in-memory cache too (see Game::AreaCache's own
	// comment) rather than released - it owns the IE::door/IE::actor
	// structs Door/Actor objects alias directly (fAreaDoor/fActor), so a
	// door's open/closed/locked state (an actor's own state is handled
	// separately, above) would otherwise reset to the file's static
	// defaults every time this area is left and re-entered. Ownership
	// transfers as-is (no extra Acquire()) - this room already held the
	// one reference GetARA()/LoadFromFile()/the cache handoff gave it.
	Game::Get()->GetAreaCache()->areas[Name()].area = fArea;
	fArea = NULL;

	// Loose items dropped on the floor here (see AddGroundItem()) - not
	// in the ARE resource, so they'd be lost otherwise. Session-only.
	Game::Get()->GetAreaCache()->areas[Name()].groundPiles = std::move(fGroundPiles);
	fGroundPiles.clear();
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


bool
AreaRoom::_IsVisibleOnScreen(const Actor* actor) const
{
	try {
		GFX::rect actorFrame = rect_to_gfx_rect(actor->Frame());
		GFX::rect mapRect = rect_to_gfx_rect(VisibleMapArea());
		if (actorFrame.Intersects(mapRect))
			return true;
	} catch (...) {
		// most likely there is no animation, so actor->Frame() throws an exception
	}
	return false;
}


bool
AreaRoom::_GetEntrance(const std::string& entranceName, IE::entrance& outEntrance) const
{
	if (!entranceName.empty()) {
		for (uint32 e = 0; e < fArea->CountEntrances(); e++) {
			IE::entrance entrance = fArea->EntranceAt(e);
			// Case insensitive comparison
			if (strcasecmp(entranceName.c_str(), entrance.name) == 0) {
				outEntrance = entrance;
				return true;
			}
		}
	} else {
		try {
			outEntrance = fArea->EntranceAt(0);
			return true;
		} catch (std::out_of_range& ex) {
			std::cerr << Log::Red << "_GetEntrance: no entrance at 0" << std::endl;
		}
	}
	return false;
}
