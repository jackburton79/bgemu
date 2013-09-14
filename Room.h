#ifndef __REGION_MAP_H
#define __REGION_MAP_H

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
class Room : public Object, public Listener {
public:
	Room();
	~Room();
	
	res_ref AreaName() const;
	WEDResource* WED();
	ARAResource* AREA() const;

	bool LoadArea(const res_ref& areaName,
					const char* longName,
					const char* entranceName = NULL);
	bool LoadArea(AreaEntry& area);

	bool LoadWorldMap();

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

	GFX::rect AreaRect() const;
	IE::point AreaOffset() const;
	void SetAreaOffset(IE::point point);
	void SetRelativeAreaOffset(IE::point point);
	void CenterArea(const IE::point& point);

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

	uint16 TileNumberForPoint(const IE::point& point);

	uint8 PointHeight(const IE::point& point) const;
	uint8 PointLight(const IE::point& point) const;
	uint8 PointSearch(const IE::point& point) const;

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleSearchMap();
	void ToggleConsole();
	void ToggleGUI();
	void ToggleDayNight();

	void CreateCreature(const char* name, IE::point where, int face);

	void DrawTile(const int16 tileNum, Bitmap *surface,
						GFX::rect tileRect, bool withOverlays);

	virtual void VideoAreaChanged(uint16 width, uint16 height);

	static Room* Get();

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _InitBitmap(GFX::rect area);
	void _InitWed(const char* name);
	void _InitBlitMask();
	void _InitHeightMap();
	void _InitLightMap();
	void _InitSearchMap();

	void _DrawBaseMap();

	void _DrawHeightMap(GFX::rect area);
	void _DrawLightMap();
	void _DrawSearchMap(GFX::rect area);
	void _DrawAnimations();
	void _DrawActors();

	void _UpdateCursor(int x, int y, int scrollByX, int scrollByY);

	Actor* _ActorAtPoint(const IE::point& point);
	Region* _RegionAtPoint(const IE::point& point);
	Container* _ContainerAtPoint(const IE::point& point);
	Object* _ObjectAtPoint(const IE::point& point);

	void _LoadOverlays();
	void _InitVariables();
	void _InitTileCells();
	void _InitAnimations();
	void _InitRegions();
	void _LoadActors();
	void _InitDoors();
	void _InitContainers();

	void _UnloadArea();
	void _UnloadWorldMap();

	res_ref fName;
	GFX::rect fViewPort; // The size of the screen area
	GFX::rect fMapArea; // the part of map which is visible. It's fViewPort
						// offsetted to fAreaOffset
	IE::point fAreaOffset;

	WEDResource *fWed;
	ARAResource *fArea;
	BCSResource *fBcs;

	// WorldMap
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	Bitmap* fBackBitmap;
	Bitmap* fBlitMask;

	Bitmap* fHeightMap;
	Bitmap* fLightMap;
	Bitmap* fSearchMap;

	int32 fMapHorizontalRatio;
	int32 fMapVerticalRatio;

	std::vector<MapOverlay*> fOverlays;
	std::vector<TileCell*> fTileCells;
	std::vector<Actor*> fActors;
	std::vector<Animation*> fAnimations;
	std::vector<Region*> fRegions;
	std::vector<Container*> fContainers;

	Actor* fSelectedActor;
	Object* fMouseOverObject;

	int fDrawSearchMap;
	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
};


#endif
