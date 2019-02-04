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
class Frame;
class MapOverlay;
class Region;
class Script;
class TileCell;
class WEDResource;
class AreaRoom : public RoomBase {
public:
	AreaRoom(const res_ref& areaName, const char* longName,
					const char* entranceName);
	~AreaRoom();
	
	WEDResource* WED();
	ARAResource* AREA() const;

	virtual GFX::rect AreaRect() const;

	::BackMap* BackMap() const;

	virtual void Draw(Bitmap *surface);
	virtual void Clicked(uint16 x, uint16 y);
	virtual void MouseOver(uint16 x, uint16 y);
	virtual void UpdateCursorAndScrolling(int x, int y,
										int scrollByX, int scrollByY);

	void DrawObject(const Object& object);
	void DrawObject(const Bitmap* bitmap, const IE::point& point, bool mask);

	uint16 TileNumberForPoint(const IE::point& point);

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

	void ToggleDayNight();

	virtual void VideoAreaChanged(uint16 width, uint16 height);

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _InitBackMap(GFX::rect area);
	void _InitWed(const char* name);
	void _InitBlitMask();
	void _InitHeightMap();
	void _InitLightMap();
	void _InitSearchMap();

	void _UpdateBaseMap(GFX::rect mapRect);

	void _DrawHeightMap(GFX::rect area);
	void _DrawLightMap();
	void _DrawSearchMap(GFX::rect area);
	void _DrawAnimations(bool advanceFrame);
	void _DrawActors();

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
	Bitmap* fSearchMap;

	typedef std::vector<Animation*> AnimationsList;
	AnimationsList fAnimations;

	typedef std::vector<Region*> RegionsList;
	RegionsList fRegions;

	typedef std::vector<Container*> ContainersList;
	ContainersList fContainers;

	Reference<Actor> fSelectedActor;
	Reference<Object> fMouseOverObject;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
	
	IE::point fCursorPosition;
};


#endif
