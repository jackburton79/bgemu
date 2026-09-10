#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Object.h"
#include "Reference.h"

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

	virtual void ReloadArea();

	virtual GFX::rect AreaRect() const;

	::BackMap* BackMap() const;
	::SearchMap* SearchMap() const;

	virtual void Update(bool runScripts); // From Object (updates scripts, etc)

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

	// Switches which actor mouse clicks/queued commands act on (party-
	// member selection) - deselects the previous one first, same
	// Select(true)/Select(false) pairing the constructor and
	// _UnloadArea() already use. NULL just deselects. A no-op if actor
	// is already the selected one.
	void SelectActor(Actor* actor);
	Actor* SelectedActor() const;

	void DrawBitmap(const Bitmap* bitmap, const IE::point& centerPoint, bool mask);

	void AddObject(Object* object);
	void RemoveObject(Object* object); // a globalId version would be nice ?

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

	void ClearAllActions();

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

	uint8 PointHeight(const IE::point& point) const;
	uint8 PointLight(const IE::point& point) const;
	uint8 PointSearch(const IE::point& point) const;

	static bool IsPointPassable(const IE::point& point);

	// Deletes every on-disk area checkpoint written by _UnloadArea()
	// (see ARAResource::WriteToFile()). Called at program shutdown for
	// now: there's no new-game-vs-load-game distinction yet, so stale
	// checkpoints from a previous run would otherwise bleed into a fresh
	// session.
	static void ClearAreaCheckpoints();

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleSearchMap();
	void ToggleConsole();

	virtual void ShowGUI();
	virtual void HideGUI();
	virtual bool IsGUIShown() const;

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
	Actor* _ActorAtPoint(const IE::point& point) const;
	Object* _ObjectAtPoint(const IE::point& point, int32& cursorIndex) const;

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

	bool _IsVisibleOnScreen(const Actor* actor) const;

	bool _GetEntrance(const std::string& entranceName,
						  IE::entrance& outEntrance) const;
	WEDResource *fWed;
	ARAResource *fArea;

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

	typedef std::vector<Effect*> EffectsList;
	EffectsList fEffects;

	ActorsList fActors;
	Reference<Actor> fSelectedActor;
	Reference<Object> fMouseOverObject;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;

	bool fCanRest = true;
};


#endif
