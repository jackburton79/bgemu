#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "RoomBase.h"

#include "Bitmap.h"
#include "IETypes.h"
#include "Listener.h"
#include "Object.h"

#include <vector>

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

	virtual void Draw();
	virtual void MouseDown(IE::point point);
	virtual void MouseMoved(IE::point point, uint32 transit);

	void DrawBitmap(const Bitmap* bitmap, const IE::point& centerPoint, bool mask);

	void AddAnimation(Animation* animation);
	void RemoveAnimation(Animation* animation);

	void AddEffect(Effect* effect);
	void RemoveEffect(Effect* effect);

	uint32 GetTileCellsForRegion(std::vector<TileCell*>& cells,
											Region* region);
	uint8 PointHeight(const IE::point& point) const;
	uint8 PointLight(const IE::point& point) const;
	uint8 PointSearch(const IE::point& point) const;

	static bool IsPointPassable(const IE::point& point);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleSearchMap();
	void ToggleConsole();

	virtual void ShowGUI();
	virtual void HideGUI();
	virtual bool IsGUIShown() const;

	virtual void VideoAreaChanged(uint16 width, uint16 height);

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

	void _UpdateBaseMap(GFX::rect mapRect);

	void _DrawHeightMap(GFX::rect area);
	void _DrawLightMap();
	void _DrawSearchMap(const GFX::rect& visibleArea);
	void _DrawAnimations(bool advanceFrame);
	void _DrawEffects();
	void _DrawActors();
	void _DrawPolygons(const GFX::rect& mapRect);

	Region* _RegionAtPoint(const IE::point& point) const;
	Container* _ContainerAtPoint(const IE::point& point);
	Object* _ObjectAtPoint(const IE::point& point, int32& cursorIndex) const;

	void _InitVariables();
	void _InitAnimations();
	void _InitRegions();
	void _LoadActors();
	void _InitDoors();
	void _InitContainers();

	void _UnloadArea();
	void _Unload();

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

	typedef std::vector<Container*> ContainersList;
	ContainersList fContainers;

	typedef std::vector<Effect*> EffectsList;
	EffectsList fEffects;

	Reference<Actor> fSelectedActor;
	Reference<Object> fMouseOverObject;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
};


#endif
