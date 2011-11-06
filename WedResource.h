#ifndef __WEDRESOURCE_H
#define __WEDRESOURCE_H

#include "Resource.h"
#include "Types.h"

#include <SDL.h>

#include <map>

struct tilemap;
class WEDResource;
class MapOverlay {
public:
	MapOverlay();
	~MapOverlay();
	
	res_ref TileSet() const;
	int16 Width() const;
	int16 Height() const;
	
	tilemap TileMapFor(int32 i);
	int16 TileIndexAt(int16 i);

	void PrintTileMaps();

private:
	res_ref fTileSet;
	
	int16 fWidth;
	int16 fHeight;

	std::map<int16, int16> fTilesIndexes;

	tilemap *fTileMaps;

	friend class WEDResource;
};


class Polygon;
class WEDResource : public Resource {
public:
	WEDResource(uint8 *data, uint32 size, uint32 key);
	WEDResource(const res_ref& name);
	~WEDResource();
	
	bool Load(TArchive *archive, uint32 key);

	SDL_Surface *GetAreaMap(bool withOverlays = false);

	int32 CountOverlays() const;
	MapOverlay *OverlayAt(int32 index);

	int32 CountPolygons() const;
	Polygon *PolygonAt(int32 index);

private:
	void _Load();
	void _LoadOverlays();
	void _LoadPolygons();

	void _DrawTile(const int16 tileNum, SDL_Surface *surface,
			SDL_Rect tileRect, bool withOverlays);
	void _DrawOverlay(SDL_Rect rect,
			SDL_Surface *surface, SDL_Surface *cell);
	static bool _IsOverlayColor(const SDL_Color &color);

	int16 _PointHeight(int16 x, int16 y);
	SDL_Color _PixelSearchColor(int16 x, int16 y);

	int32 fNumOverlays;
	int32 fNumDoors;
	int32 fNumPolygons;
	Polygon *fPolygons;
	SDL_Surface *fSearchMap;
	SDL_Surface *fHeightMap;

	int16 fHAspect;
	int16 fVAspect;
	
	MapOverlay *fOverlays;

	int32 fOverlaysOffset;
	int32 f2ndHeaderOffset;
	int32 fDoorsOffset;
	int32 fDoorTileCellsOffset;


};


#endif
