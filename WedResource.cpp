#include "BmpResource.h"
#include "Door.h"
#include "MemoryStream.h"
#include "Graphics.h"
#include "Polygon.h"
#include "ResManager.h"
#include "Tile.h"
#include "TisResource.h"
#include "WedResource.h"
#include "Utils.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <SDL.h>

using namespace std;

struct overlay {
	int16 width;
	int16 height;
	res_ref resource_ref;
	uint32 unk;
	uint32 tilemap_offset;
	uint32 tile_lookup_offset;
};


struct tilemap {
	int16 primary_tile_index;
	int16 primary_tile_count;
	int16 secondary_tile_index;
	int8 mask;
	int8 unk1;
	int8 unk2;
	int8 unk3;
};




WEDResource::WEDResource(const res_ref &name)
	:
	Resource(name, RES_WED),
	fNumPolygons(0),
	fPolygons(NULL)
{
}


WEDResource::~WEDResource()
{
	delete[] fPolygons;
}


void
WEDResource::_Load()
{
	if (!CheckSignature("WED "))
		throw -1;

	if (!CheckVersion("V1.3"))
		throw -1;
	
	fData->ReadAt(8, fNumOverlays);
	fData->ReadAt(12, fNumDoors);
	fData->ReadAt(16, fOverlaysOffset);
	fData->ReadAt(20, f2ndHeaderOffset);
	fData->ReadAt(24, fDoorsOffset);
	fData->ReadAt(28, fDoorTileCellsOffset);

	_LoadPolygons();
}


bool
WEDResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	_Load();

	return true;
}


uint32
WEDResource::CountOverlays() const
{
	return fNumOverlays;
}


void
WEDResource::_ReadTileMap(overlay overlay, uint32 &x, MapOverlay *mapOverlay)
{
    uint32 mapOffset = overlay.tilemap_offset + x * sizeof (tilemap);
    tilemap tileMap;
    fData->ReadAt(mapOffset, tileMap);
    mapOverlay->fTileMaps[x].SetMask(tileMap.mask);
    mapOverlay->fTileMaps[x].SetOverlay(mapOverlay);
    const int32 indexCount = tileMap.primary_tile_count;
    const int32 offset = overlay.tile_lookup_offset + (tileMap.primary_tile_index * sizeof(uint16));
    int16 tisIndex;
    for (int32 c = 0;c < indexCount;c++) {
        fData->ReadAt(offset + c * sizeof(int16), tisIndex);
        mapOverlay->fTileMaps[x].AddTileIndex(tisIndex);
    }

    const int32 secondaryOffset = overlay.tile_lookup_offset + (tileMap.secondary_tile_index * sizeof(uint16));
    fData->ReadAt(secondaryOffset, tisIndex);
    mapOverlay->fTileMaps[x].SetSecondaryTileIndex(tisIndex);
}


MapOverlay *
WEDResource::GetOverlay(uint32 index)
{
	if (index < 0 || index >= fNumOverlays)
		return NULL;

	MapOverlay *mapOverlay = new MapOverlay();

	fData->Seek(fOverlaysOffset + index * sizeof(overlay), SEEK_SET);

	::overlay overlay;
	fData->Read(overlay);

	mapOverlay->fTileSet = overlay.resource_ref;
	mapOverlay->fWidth = overlay.width;
	mapOverlay->fHeight = overlay.height;
	mapOverlay->fTileMaps = new TileMap[overlay.width * overlay.height];

	const uint32 overlaySize = overlay.height * overlay.width;
	for (uint32 x = 0; x < overlaySize; x++)
		_ReadTileMap(overlay, x, mapOverlay);

	return mapOverlay;
}


Polygon *
WEDResource::PolygonAt(uint32 index)
{
	return &fPolygons[index];
}


uint32
WEDResource::CountPolygons() const
{
	return fNumPolygons;
}


uint32
WEDResource::CountDoors() const
{
	return fNumDoors;
}


Door *
WEDResource::GetDoor(uint32 index)
{
	if (index < 0 || index >= fNumDoors)
		return NULL;

	fData->Seek(fDoorsOffset + index * sizeof(door_wed), SEEK_SET);

	door_wed wedDoor;
	fData->Read(wedDoor);
	wedDoor.Print();

	Door *newDoor = new Door(&wedDoor);

	fData->Seek(fDoorTileCellsOffset + wedDoor.cell_index * sizeof(uint16));
	for (uint16 i = 0; i < wedDoor.cell_count; i++) {
		uint16 tileIndex;
		fData->Read(tileIndex);
		newDoor->fTilesOpen.push_back(tileIndex);
	}
	printf("\n");

	return newDoor;
}


void
WEDResource::_LoadPolygons()
{
	assert(fPolygons == NULL);

	fData->Seek(f2ndHeaderOffset, SEEK_SET);

	fData->Read(fNumPolygons);

	fPolygons = new Polygon[fNumPolygons];

	int32 polygonsOffset;
	fData->Read(polygonsOffset);

	int32 verticesOffset;
	int32 wallGroups;
	int32 indexTableOffset;

	fData->Read(verticesOffset);
	fData->Read(wallGroups);
	fData->Read(indexTableOffset);

	for (uint32 p = 0; p < fNumPolygons; p++) {
		fData->Seek(polygonsOffset + p * sizeof(polygon), SEEK_SET);
		::polygon polygon;
		fData->Read(polygon);
		fPolygons[p].SetFrame(polygon.x_min, polygon.x_max,
				polygon.y_min, polygon.y_max);
		fData->Seek(polygon.vertex_index * sizeof(point) + verticesOffset, SEEK_SET);
		for (int i = 0; i < polygon.vertices_count; i++) {
			point vertex;
			fData->Read(vertex);
			fPolygons[p].AddPoints(&vertex, 1);
		}
	}
}


/* static */
bool
WEDResource::_IsOverlayColor(const SDL_Color &color)
{
	return color.r == 0 and color.g == 0 and color.b == 255;
}


SDL_Color
WEDResource::_PixelSearchColor(int16 x, int16 y)
{
	/*const int32 searchMapX = x / fHAspect;
	const int32 searchMapY = y / fVAspect;
	const int32 pitch = fSearchMap->pitch;
	SDL_LockSurface(fSearchMap);
	const SDL_Color *colors = fSearchMap->format->palette->colors;
	uint8 *pixels = (Uint8*)(fSearchMap->pixels);
	const int32 pixelIndex = searchMapY * pitch + searchMapX;
	const SDL_Color color = colors[pixels[pixelIndex]];
	SDL_UnlockSurface(fSearchMap);

	//printf("pixel: %d\n", pixels[pixelIndex]);

	return color;*/
	SDL_Color color;
	return color;
}


int16
WEDResource::_PointHeight(int16 x, int16 y)
{
	/*const int32 mapX = x / fHAspect;
	const int32 mapY = y / fVAspect;
	const int32 pitch = fHeightMap->pitch;
	SDL_LockSurface(fHeightMap);
	const SDL_Color *colors = fHeightMap->format->palette->colors;
	uint8 *pixels = (Uint8*)(fHeightMap->pixels);
	const int32 pixelIndex = mapY * pitch + mapX;
	const SDL_Color color = colors[pixels[pixelIndex]];
	SDL_UnlockSurface(fHeightMap);

	//uint32 pix = SDL_MapRGB(color.r, color.g, color.b);
	if (color.r == 128 and color.g == 128 and color.b == 128)
		return 0;
*/
	return 1;
}


// MapOverlay
MapOverlay::MapOverlay()
	:
	fTileMaps(NULL)
{
}


MapOverlay::~MapOverlay()
{
	delete[] fTileMaps;
}


res_ref
MapOverlay::TileSet() const
{
	return fTileSet;
}


uint16
MapOverlay::Width() const
{
	return fWidth;
}


uint16
MapOverlay::Height() const
{
	return fHeight;
}


uint16
MapOverlay::Size() const
{
	return fWidth * fHeight;
}


TileMap *
MapOverlay::TileMapForTile(int32 i)
{
	// TODO: handle this more correctly
	if (i >= fWidth * fHeight) {
		printf("requested tile %d, max %d\n", i, Size());
		i = (fWidth * fHeight) - 1;
	}

	return &fTileMaps[i];
}



void
MapOverlay::PrintTileMaps()
{
	/*for (int32 x = 0; x < fHeight * fWidth; x++) {
		printf("tilemap[%d]:\n", x);
		printf("\tp_index: %d\n", fTileMaps[x].primary_tile_index);
		printf("\tp_count: %d\n", fTileMaps[x].primary_tile_count);
		printf("\ts_index: %d\n", fTileMaps[x].secondary_tile_index);
		printf("\tmask: %d\n", fTileMaps[x].mask);
	}*/
}


