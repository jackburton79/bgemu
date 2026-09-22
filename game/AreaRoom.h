#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Object.h"
#include "Reference.h"
#include "Variables.h"

#include <vector>

typedef std::vector<Actor*> ActorsList;

class Actor;
class Animation;
class ARAResource;
class BCSResource;
class BackMap;
class Bitmap;
class Container;
class Door;
class Effect;
class Frame;
class MapOverlay;
class Region;
class Script;
class SearchMap;
class TileCell;
class WEDResource;
class AreaRoom : public RoomBase {
public:
	AreaRoom(const res_ref& areaName, const char* longName,
					const char* entranceName);

	WEDResource* WED() const;
	ARAResource* AREA() const;

	// This area's own MYAREA-scoped variable table (IESDP: "each area can
	// have its own copy of the variable") - kept separate from Core::Vars()
	// (GLOBAL) so a MYAREA variable can never collide with a same-named
	// GLOBAL one. See Variables::GetScoped()/SetScoped().
	Variables& AreaVars();

	virtual void ReloadArea();

	// Rebuilds the GUIW screen (HUD, portraits, area-name label) and
	// re-grafts this room into it - see Core::ReturnFromWorldMap(),
	// the only caller: this area was backgrounded (not unloaded) while
	// the world map was up, and DetachFromWindow() already pulled it out
	// of the now-destroyed GUIW window, so there's nothing to rebuild
	// besides the GUI chrome itself (actors/scripts/etc. were never torn
	// down).
	virtual void Resume();

	virtual GFX::rect AreaRect() const;

	::BackMap* BackMap() const;
	::SearchMap* SearchMap() const;

	virtual void Update(bool runScripts); // From Object (updates scripts, etc)

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseUp(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);
	// Headless-test entry point (Click-Area console command): same click
	// dispatch as MouseDown(), but point is already in area coordinates -
	// skips the screen->area conversion, which needs a real camera/window.
	void ClickAt(IE::point areaPoint);
	// Headless-test entry point (Drag-Select console command) for the
	// same rectangle-select MouseUp() does at the end of a real mouse
	// drag - both points already in area coordinates.
	void DragSelectAt(IE::point areaStart, IE::point areaEnd, bool additive);

	// Switches which actor mouse clicks/queued commands act on (party-
	// member selection): deselects everyone else, selects just `actor`,
	// same Select(true)/Select(false) pairing the constructor and
	// _UnloadArea() already use. NULL just clears the selection. A no-op
	// if `actor` is already the only one selected.
	void SelectActor(Actor* actor);
	// Replaces the whole selection with exactly these actors (drag-rect
	// select's "replace" mode) - anyone currently selected but not in
	// `actors` is deselected, everyone in `actors` not already selected
	// is selected (in the given order). Only party members are ever
	// added, matching the real engine (an NPC can't be player-selected);
	// anyone else in `actors` is silently skipped.
	void SetSelectedActors(const ActorsList& actors);
	// Adds every party member of `actors` to the current selection
	// (shift+drag) without touching anyone else's selection state.
	void AddToSelection(const ActorsList& actors);
	// Adds `actor` to the selection if it wasn't already selected,
	// removes it otherwise (shift-click, on the map or on its HUD
	// portrait) - leaves everyone else's selection untouched. A no-op
	// for anyone but a party member.
	void ToggleSelected(Actor* actor);
	// The "primary" selected actor: the first one still selected, in the
	// order each was added to the selection - what a single-target
	// command (dialog, the minimap marker, a console command, ...) acts
	// on. NULL if nothing is selected.
	Actor* SelectedActor() const;
	// Every currently selected actor, in that same primary-first order.
	void GetSelectedActors(ActorsList& actors) const;
	uint32 CountSelectedActors() const;

	void DrawBitmap(const Bitmap* bitmap, const IE::point& centerPoint, bool mask);

	void AddObject(Object* object);
	void RemoveObject(Object* object); // a globalId version would be nice ?

	// `actor` was one of this area's own placed creatures and now belongs
	// to the Game (party member, global NPC): the area won't place it again.
	void ForgetPlacedActor(Actor* actor);

	void AddAnimation(Animation* animation);
	void RemoveAnimation(Animation* animation);

	void AddEffect(Effect* effect);
	void RemoveEffect(Effect* effect);

	// Objects
	int32 GetActorsList(ActorsList& list) const;
	int32 ActorsCount() const;
	Actor* ActorAt(int32 index) const;

	Object* GetObject(const char* name) const;
	Object* GetObject(uint16 globalEnum) const;
	Actor* GetObjectFromNode(object_params* node) const;
	Actor* GetObject(const Region* region) const;
	Actor* GetNearestEnemyOf(const Actor* object) const;
	Actor* GetNearestEnemyOfType(const Actor* object, int ieClass) const;
	Region* RegionAtPoint(const IE::point& point) const;

	// Stops every actor in the area *except* `keep` (the actor that
	// invoked ClearAllActions(), if any) - see the .cpp for why the
	// exception matters.
	void ClearAllActions(Object* keep = NULL);

	uint32 GetTileCellsForRegion(std::vector<TileCell*>& cells,
											Region* region);

	int Distance(const Object* a, const Object* b) const;

	// Whether nothing along the straight line between the two points
	// blocks light (see SearchMap::BlocksLight()) - used by Actor::
	// CanSee() so See()/creature perception stop seeing through walls.
	bool HasLineOfSight(const IE::point& from, const IE::point& to) const;
	// test_function (see PathFind.h) for HasLineOfSight() above - reads
	// the currently active room the same way IsPointPassable() below
	// already does (a plain function pointer can't capture `this`).
	static bool PointDoesNotBlockLight(const IE::point& point);

	const std::vector<Door*>& Doors() const;
	const std::vector<Container*>& Containers() const;

	// Loose items on the floor (dropped from the inventory - see
	// Game::DropHeldItemOnGround()). Session-only: carried across
	// leaving/re-entering the area by Game::AreaCache, never written to
	// disk. Dropping near an existing pile merges into it.
	void AddGroundItem(const IE::item& item, const IE::point& position);
	const std::vector<IE::ground_pile>& GroundPiles() const;
	void SetGroundPiles(std::vector<IE::ground_pile> piles);
	// Index into GroundPiles() of the pile whose icon covers `areaPoint`
	// (world click) or that merges with it (a drop, or - Game::
	// _UpdateGroundItemSlots() - the GUIINV ground-item row, keyed off
	// the shown character's own position), or -1. `areaPoint` is in area
	// coordinates.
	int32 GroundPileAtPoint(const IE::point& areaPoint) const;
	// Auto-loots the pile at `index` into `taker`'s inventory; whatever
	// doesn't fit stays behind, an emptied pile is removed.
	void PickUpGroundPile(size_t index, Actor* taker);

	uint8 PointHeight(const IE::point& point) const;
	uint8 PointLight(const IE::point& point) const;
	uint8 PointSearch(const IE::point& point) const;

	static bool IsPointPassable(const IE::point& point);

	// Every on-disk area checkpoint (see _UnloadArea()/WriteCheckpoint())
	// lives under this single directory for the session's entire
	// lifetime, regardless of which save (if any) is currently loaded -
	// same idea as real IE's own single cache directory (GemRB's
	// Interface::CachePath), which a save's own .SAV archive is
	// extracted into on load and re-archived from on save. Game::Save()/
	// Load() copy this directory's contents to/from a specific save
	// file's own ".arecache" directory (see their own comments) instead
	// of ever pointing AreaRoom itself at a different directory - every
	// area checkpoint this engine ever writes, for any save or none,
	// goes through this one path.
	static const char* AreaCheckpointDir();

	// Deletes every on-disk area checkpoint in AreaCheckpointDir().
	// Called at program shutdown so a later, unrelated session never
	// inherits this one's checkpoints; also called before entering the
	// game loop, in case an earlier run crashed before reaching shutdown.
	static void ClearAreaCheckpoints();

	// Snapshots this area's own current state to disk right now, without
	// unloading it - unlike _UnloadArea()'s own checkpoint write (only
	// reached by actually leaving), this is what lets Game::Save() capture
	// the area the party is standing in *right now*, which would
	// otherwise come back pristine on a later load (nothing else writes a
	// checkpoint for the current, still-loaded area).
	void WriteCheckpoint();

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleSearchMap();
	void ToggleConsole();

	// SETAREARESTFLAG(I:CanRest*) - defaults to true, not initialized
	// from the ARE header's own "Rest disabled" bit (AREAFLAG.IDS bit 1)
	// - that flag isn't parsed by this engine yet, so this is purely a
	// runtime override for now.
	bool CanRest() const;
	void SetCanRest(bool canRest);

private:
	virtual ~AreaRoom();
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _InitBackMap(const GFX::rect& area);
	void _InitWed();
	void _InitBlitMask();
	void _InitHeightMap();
	void _InitLightMap();
	void _InitSearchMap();

	void _CleanDestroyedObjects();

	void _UpdateBaseMap(GFX::rect mapRect);

	void _DrawHeightMap(GFX::rect area);
	void _DrawLightMap();
	void _DrawSearchMap(const GFX::rect& visibleArea);
	void _DrawPolygons(const GFX::rect& mapRect);

	// Drawing
	void _DrawAnimations(bool advanceFrame);
	void _DrawEffects();
	void _DrawActors();

	Container* _ContainerAtPoint(const IE::point& point) const;
	void _DrawGroundPiles();
	Actor* _ActorAtPoint(const IE::point& point) const;
	Object* _ObjectAtPoint(const IE::point& point, int32& cursorIndex) const;
	// Shared by MouseUp() and ClickAt() - point already in area coords.
	void _HandleClickAt(IE::point point);
	// Queues a MOVETOPOINT for every selected actor towards `point` (each
	// spread to its own nearby point, see _SpreadPoint() in the .cpp) -
	// the fallback "walk there" click, and (now) also a travel region
	// click. No-op if nothing is selected.
	void _QueueMoveToPoint(IE::point point);
	// Shared by MouseUp() and DragSelectAt() - both points already in
	// area coords.
	void _FinishDragSelect(const IE::point& areaStart, const IE::point& areaEnd,
		bool additive);

	void _InitVariables();
	void _InitAnimations();
	void _InitRegions();
	// revisited: true if this area was left (and cached, see Game::
	// AreaCache) earlier this session - restores its own non-party
	// actors from that cache instead of parsing fresh ones from fArea,
	// even when the cached list is empty (everyone there died/left last
	// time - a fresh parse would wrongly resurrect them).
	void _LoadActors(bool revisited);
	void _InitDoors();
	void _InitContainers();

	void _UnloadArea();
	// Shared by _UnloadArea() and WriteCheckpoint() - re-embeds one still-
	// alive actor's current CRE bytes (HP, inventory, spellbook, status)
	// into fArea's own embedded-CRE table, the only part of a live actor's
	// state ARAResource::WriteToFile() doesn't already see for free
	// (position/orientation/door state alias fArea's own structs
	// directly - see Actor::fActor/Door::fAreaDoor).
	void _EmbedActorCRE(Actor* actor);

	// The GUIW rebuild shared by the constructor (first load) and
	// Resume() (returning from the world map): loads GUIW, grafts this
	// room into its window as the map viewport, restores the area-name
	// label from fLongName, and refreshes the HUD portraits.
	void _SetupGUI();

	bool _IsVisibleOnScreen(const Actor* actor) const;

	bool _GetEntrance(const std::string& entranceName,
						  IE::entrance& outEntrance) const;
	WEDResource *fWed;
	ARAResource *fArea;
	std::string fLongName;

	::BackMap* fBackMap;
	Bitmap* fBlitMask;

	Bitmap* fHeightMap;
	Bitmap* fLightMap;
	::SearchMap* fSearchMap;

	typedef std::vector<Animation*> AnimationsList;
	AnimationsList fAnimations;

	typedef std::vector<Region*> RegionsList;
	RegionsList fRegions;

	typedef std::vector<Door*> DoorsList;
	DoorsList fDoors;

	typedef std::vector<Container*> ContainersList;
	ContainersList fContainers;

	std::vector<IE::ground_pile> fGroundPiles;

	Variables fAreaVariables;

	typedef std::vector<Effect*> EffectsList;
	EffectsList fEffects;

	ActorsList fActors;
	// In selection order (the first entry is SelectedActor(), the
	// "primary") - see SelectActor()/ToggleSelected()/SetSelectedActors().
	std::vector<Reference<Actor>> fSelectedActors;
	Reference<Object> fMouseOverObject;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;

	bool fCanRest = true;

	// Drag-select bookkeeping (MouseDown()/MouseMoved()/MouseUp()), both
	// points in the same control-local space Draw() itself uses for a
	// screen overlay (see MouseDown()'s own comment) - fDragOrigin is
	// where the current mouse-button press started, fDragCurrent where
	// the pointer is now. Dragging only actually starts (fDragging) once
	// the pointer has moved past a small threshold, so a plain click
	// (press + release with barely any movement) still reaches
	// _HandleClickAt() as a normal click instead of always selecting a
	// near-empty rectangle. fDragAdditive is whether shift was held when
	// the press started (shift+drag adds to the existing selection
	// instead of replacing it, same convention as a shift+click).
	IE::point fDragOrigin = { 0, 0 };
	IE::point fDragCurrent = { 0, 0 };
	bool fDragging = false;
	bool fDragAdditive = false;
	// Whether MouseUp() still owes a click/drag dispatch for the MouseDown()
	// that started this gesture - false for a target-mode click, which
	// MouseDown() already handled immediately and returned from without
	// capturing the mouse; MouseUp() still gets called right after (the
	// point still lands on this room), and without this flag would dispatch
	// _HandleClickAt() a second time for the very same click.
	bool fAwaitingMouseUp = false;
};


#endif
