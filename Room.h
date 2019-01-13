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
class AreaEntry;
class BCSResource;
class BackMap;
class Bitmap;
class Container;
class Door;
class Frame;
class MapOverlay;
class MOSResource;
class Region;
class Script;
class TileCell;
class WEDResource;
class WMAPResource;
class AreaRoom : public RoomContainer {
public:
	static bool Create();
	static void Delete();
	
	AreaRoom(const res_ref& areaName, const char* longName,
					const char* entranceName);
	~AreaRoom();
	
	WEDResource* WED();
	ARAResource* AREA() const;

	virtual IE::rect Frame() const;

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

	GFX::rect AreaRect() const;
	IE::point AreaOffset() const;
	GFX::rect VisibleArea() const;

	void SetAreaOffset(IE::point point);
	void SetRelativeAreaOffset(IE::point point);
	void CenterArea(const IE::point& point);

	virtual ::BackMap* BackMap() const;

	void ConvertToArea(GFX::rect& rect);
	void ConvertToArea(IE::point& point);
	void ConvertFromArea(GFX::rect& rect);
	void ConvertFromArea(IE::point& point);

	void ConvertToScreen(IE::point& point);
	void ConvertToScreen(GFX::rect& rect);

	void Draw(Bitmap *surface);
	void Clicked(uint16 x, uint16 y);
	void MouseOver(uint16 x, uint16 y);

	void DrawObject(const Object& object);
	void DrawObject(const Bitmap* bitmap, const IE::point& point, bool mask);

	void ActorEnteredArea(const Actor* actor);
	void ActorExitedArea(const Actor* actor);

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
	void ToggleGUI();
	void ToggleDayNight();

	virtual void VideoAreaChanged(uint16 width, uint16 height);

	static AreaRoom* Get();

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

	void _UpdateCursor(int x, int y, int scrollByX, int scrollByY);

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
	void _UnloadWorldMap();
	void _Unload();

	GFX::rect fScreenArea;
	GFX::rect fMapArea; // the part of map which is visible. It's fScreenArea
						// offsetted to fAreaOffset
	IE::point fAreaOffset;

	WEDResource *fWed;
	ARAResource *fArea;
	BCSResource *fBcs;

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	::BackMap* fBackMap;
	Bitmap* fBlitMask;

	Bitmap* fHeightMap;
	Bitmap* fLightMap;
	Bitmap* fSearchMap;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	std::vector<Animation*> fAnimations;
	std::vector<Reference<Region> > fRegions;
	std::vector<Reference<Container> > fContainers;

	Reference<Actor> fSelectedActor;
	Reference<Object> fMouseOverObject;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
};


#endif
