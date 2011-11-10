#ifndef __WEDRESOURCE_H
#define __WEDRESOURCE_H

#include "Resource.h"
#include "Types.h"

#include <SDL.h>

#include <map>

struct wed_door {

};


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
	
	TileMap *TileMapForTile(int32 i);

	void PrintTileMaps();

private:
	res_ref fTileSet;
	
	uint16 fWidth;
	uint16 fHeight;

	TileMap *fTileMaps;

	friend class WEDResource;
};


class Polygon;
class WEDResource : public Resource {
public:
	WEDResource(const res_ref& name);
	~WEDResource();
	
	bool Load(Archive *archive, uint32 key);

	int32 CountOverlays() const;
	MapOverlay *OverlayAt(int32 index);

	int32 CountPolygons() const;
	Polygon *PolygonAt(int32 index);

	void DrawTile(const int16 tileNum, SDL_Surface *surface,
					SDL_Rect tileRect, bool withOverlays);

private:
	void _Load();
	void _LoadOverlays();
	void _LoadPolygons();

	void _DrawOverlay(SDL_Surface *surface, SDL_Surface *cell,
			SDL_Rect rect, SDL_Color *transparent);
	static bool _IsOverlayColor(const SDL_Color &color);

	int16 _PointHeight(int16 x, int16 y);
	SDL_Color _PixelSearchColor(int16 x, int16 y);

	int32 fNumOverlays;
	int32 fNumDoors;
	int32 fNumPolygons;
	Polygon *fPolygons;
	
	MapOverlay *fOverlays;

	int32 fOverlaysOffset;
	int32 f2ndHeaderOffset;
	int32 fDoorsOffset;
	int32 fDoorTileCellsOffset;
};


#endif
