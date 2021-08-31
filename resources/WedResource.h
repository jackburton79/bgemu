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
	MapOverlay(uint16 width, uint16 height, const res_ref& tileSet);
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

	std::map<uint32, TileMap*> fTileMaps;

	friend class WEDResource;
};

namespace GFX {
	struct Color;
}

struct overlay;
class Door;
class Polygon;
class WEDResource : public Resource {
public:
	WEDResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	uint32 CountOverlays() const;
	MapOverlay *GetOverlay(uint32 index);

	uint32 CountPolygons() const;
	const Polygon *PolygonAt(uint32 index) const;

	uint32 CountDoors() const;
	
	bool LinkDoorWithTiledObject(Door* door);
	
private:
	virtual ~WEDResource();
	void _Load();
	void _LoadPolygons();
	void _ReadTileMap(overlay overlay, const uint32 &x, MapOverlay *mapOverlay);
	
	uint32 fNumOverlays;
	uint32 fNumTiledObjects;
	uint32 fNumPolygons;
	Polygon *fPolygons;
	
	MapOverlay *fOverlays;

	uint32 fOverlaysOffset;
	uint32 f2ndHeaderOffset;
	uint32 fTiledObjectsOffset;
	uint32 fTiledObjectsTileCellsOffset;
};


#endif
