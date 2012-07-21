#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "SDL.h"
#include "IETypes.h"
#include "Object.h"

#include <vector>

class Actor;
class Animation;
class ARAResource;
class Bitmap;
class Door;
class MapOverlay;
class MOSResource;
class Script;
class TileCell;
class WMAPResource;

class Room : public Object {
public:
	Room();
	~Room();
	
	bool Load(const char* areaName);
	bool LoadWorldMap();

	GFX::rect ViewPort() const;
	void SetViewPort(GFX::rect rect);

	GFX::rect AreaRect() const;

	void Draw(Bitmap *surface);
	void Clicked(uint16 x, uint16 y);
	void MouseOver(uint16 x, uint16 y);
	uint16 TileNumberForPoint(uint16 x, uint16 y);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleLightMap();
	void ToggleSearchMap();
	void ToggleHeightMap();

	void CreateCreature(const char* name, IE::point where, int face);

	void DrawTile(const int16 tileNum, Bitmap *surface,
						GFX::rect tileRect, bool withOverlays);

	void DumpOverlays(const char *path);

private:
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

	res_ref fName;
	GFX::rect fVisibleArea;

	WEDResource *fWed;
	ARAResource *fArea;
	BCSResource *fBcs;
	WMAPResource* fWorldMap;
	BAMResource* fWorldMapIcons;
	MOSResource* fWorldMapBackground;
	Bitmap*	fWorldMapBitmap;

	std::vector<MapOverlay*> fOverlays;
	std::vector<TileCell*> fTileCells;
	std::vector<Animation*> fAnimations;

	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawLightMap;
	bool fDrawSearchMap;
	bool fDrawHeightMap;
	bool fDrawAnimations;

};


#endif
