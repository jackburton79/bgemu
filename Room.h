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
class Door;
class MapOverlay;
class MOSResource;
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

	bool LoadArea(const res_ref& areaName);
	bool LoadArea(AreaEntry& area);
	bool LoadWorldMap();

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

	GFX::rect AreaRect() const;
	void GetAreaOffset(IE::point& point);
	void SetAreaOffset(IE::point point);

	void ConvertToArea(GFX::rect& rect);
	void ConvertToArea(IE::point& point);
	void ConvertFromArea(GFX::rect& rect);
	void ConvertFromArea(IE::point& point);

	void ConvertToScreen(IE::point& point);
	void ConvertToScreen(GFX::rect& rect);

	void Draw(Bitmap *surface);
	void Clicked(uint16 x, uint16 y);
	void MouseOver(uint16 x, uint16 y);
	uint16 TileNumberForPoint(const IE::point& point);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleConsole();

	void CreateCreature(const char* name, IE::point where, int face);

	void DrawTile(const int16 tileNum, Bitmap *surface,
						GFX::rect tileRect, bool withOverlays);

	//void GetPolygons(GFX::rect, IE::polygon *poly);
	virtual void VideoAreaChanged(uint16 width, uint16 height);

	static Room* CurrentArea();

private:
	void _DrawConsole();
	GFX::rect _ConsoleRect() const;

	void _InitBitmap(GFX::rect area);
	void _DrawBaseMap(GFX::rect area);

	void _DrawLightMap();
	void _DrawSearchMap(GFX::rect area);
	void _DrawHeightMap(GFX::rect area);
	void _DrawAnimations(GFX::rect area);
	void _DrawActors(GFX::rect area);

	void _LoadOverlays();
	void _InitVariables();
	void _InitTileCells();
	void _InitAnimations();
	void _LoadActors();
	void _InitDoors();

	void _UnloadArea();
	void _UnloadWorldMap();

	res_ref fName;
	GFX::rect fViewPort;
	IE::point fAreaOffset;

	WEDResource *fWed;
	ARAResource *fArea;
	BCSResource *fBcs;
	WMAPResource* fWorldMap;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;
	Bitmap* fBackBitmap;

	std::vector<MapOverlay*> fOverlays;
	std::vector<TileCell*> fTileCells;
	std::vector<Animation*> fAnimations;

	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawAnimations;
	bool fShowingConsole;
};


#endif
