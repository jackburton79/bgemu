#ifndef __WEDRESOURCE_H
#define __WEDRESOURCE_H

#include "Resource.h"
#include "IETypes.h"

#include <SDL.h>

#include <map>

struct tilemap;
class TileMap;
class WEDResource;
class MapOverlay {
public:
	MapOverlay();
	~MapOverlay();
	
	res_ref TileSet() const;
	uint16 Width() const;
	uint16 Height() const;
	uint16 Size() const;

	TileMap *TileMapForTileCell(int32 i);
	void PrintTileMaps();

private:
	res_ref fTileSet;
	
	uint16 fWidth;
	uint16 fHeight;

	TileMap *fTileMaps;

	friend class WEDResource;
};


struct overlay;
class Polygon;
class WEDResource : public Resource {
public:
	WEDResource(const res_ref& name);
	~WEDResource();
	
	bool Load(Archive *archive, uint32 key);

	uint32 CountOverlays() const;
	MapOverlay *GetOverlay(uint32 index);

	uint32 CountPolygons() const;
	Polygon *PolygonAt(uint32 index);

	uint32 CountDoors() const;
	Door *GetDoor(uint32 index);

private:
	void _Load();
	void _LoadPolygons();

	static bool _IsOverlayColor(const SDL_Color &color);

	int16 _PointHeight(int16 x, int16 y);
	SDL_Color _PixelSearchColor(int16 x, int16 y);
    void _ReadTileMap(overlay overlay, const uint32 &x, MapOverlay *mapOverlay);

	uint32 fNumOverlays;
	uint32 fNumDoors;
	uint32 fNumPolygons;
	Polygon *fPolygons;
	
	MapOverlay *fOverlays;

	uint32 fOverlaysOffset;
	uint32 f2ndHeaderOffset;
	uint32 fDoorsOffset;
	uint32 fDoorTileCellsOffset;
};


#endif
