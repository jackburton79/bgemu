#include "BmpResource.h"
#include "MemoryStream.h"
#include "Graphics.h"
#include "Polygon.h"
#include "ResManager.h"
#include "TileCell.h"
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


class TileMap {
public:
	TileMap();

	void AddTileIndex(uint16 index);
	uint16 TileIndex();

	void SetSecondaryTileIndex(uint16 index);
	uint16 SecondaryTileIndex() const;

	uint8 Mask() const;
	void SetMask(uint8 mask);

private:
	std::vector<int16> fIndices;
	int16 fSecondaryIndex;
	uint16 fCurrentIndex;
	uint8 fMask;
};


WEDResource::WEDResource(const res_ref &name)
	:
	Resource(name, RES_WED),
	fNumPolygons(0),
	fPolygons(NULL),
	fOverlays(NULL)
{
}


WEDResource::~WEDResource()
{
	delete[] fPolygons;
	delete[] fOverlays;
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

	_LoadPolygons();
	_LoadOverlays();
}


bool
WEDResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	_Load();
	return true;
}


void
WEDResource::_DrawOverlay(SDL_Surface *surface, SDL_Surface *cell,
		SDL_Rect rect, SDL_Color *color)
{
	if (color) {
		SDL_SetColorKey(cell, SDL_SRCCOLORKEY,
				SDL_MapRGB(cell->format, color->r, color->g, color->b));
	}

	SDL_BlitSurface(cell, NULL, surface, &rect);
}


void
WEDResource::DrawTile(const int16 tileNum, SDL_Surface *surface,
		SDL_Rect tileRect, bool withOverlays)
{
	TileMap *tileMap = OverlayAt(0)->TileMapForTile(tileNum);
    int maxOverlay = withOverlays ? fNumOverlays : 1;

    // Green is the colorkey
    // TODO: only for BG, it seems
    SDL_Color trans = { 0, 255, 0 };

    // Draw the overlays from top to bottom:
    // this way we can draw the base overlay using
    // the correct colorkey, and the overlays (water, lava, etc)
    // are rendered correctly
    for (int i = maxOverlay - 1; i >= 0; i--) {
    	// Check if this overlay needs to be drawn
    	if (i != 0 && (tileMap->Mask() & (1 << i)) == 0)
    		continue;

    	MapOverlay *nextOverlay = OverlayAt(i);
   		if (nextOverlay == NULL)
   			break;

   		TileMap *map = nextOverlay->TileMapForTile(tileNum);
   		const uint16 index = map->TileIndex();
   		//const uint16 index = map->SecondaryTileIndex();
   		TISResource *tis = gResManager->GetTIS(nextOverlay->TileSet());
   		SDL_Surface *cell = tis->TileCellAt(index);
   		SDL_Color *color = NULL;
   		if (i == 0 && tileMap->Mask() != 0) {
   			/*for (int32 i = 0; i < 256; i++) {
   				SDL_Color *c = &cell->format->palette->colors[i];
   				printf("color[%d]: %d %d %d %d\n", i,
   						c->r, c->g, c->b, c->unused);
   			}*/
   			color = &trans;
   			//color = &cell->format->palette->colors[255];
   		}
   		_DrawOverlay(surface, cell, tileRect, color);
   		SDL_FreeSurface(cell);

   		gResManager->ReleaseResource(tis);
   	}
}

/*
SDL_Surface *
WEDResource::GetAreaMap(bool withOverlays, bool withPolygons)
{
	if (withPolygons) {
		for (int32 p = 0; p < CountPolygons(); p++) {
			Graphics::DrawPolygon(*PolygonAt(p), surface);
		}
	}

	return surface;
}
*/

int32
WEDResource::CountOverlays() const
{
	return fNumOverlays;
}


MapOverlay *
WEDResource::OverlayAt(int32 index)
{
	if (index < 0 || index >= fNumOverlays)
		return NULL;

	return &fOverlays[index];
}


Polygon *
WEDResource::PolygonAt(int32 index)
{
	return &fPolygons[index];
}


int32
WEDResource::CountPolygons() const
{
	return fNumPolygons;
}


void
WEDResource::_LoadOverlays()
{
	assert(fOverlays == NULL);

	fOverlays = new MapOverlay[fNumOverlays];
	for (int32 index = 0; index < fNumOverlays; index++) {
		fData->Seek(fOverlaysOffset + index * sizeof(overlay), SEEK_SET);

		::overlay overlay;
		fData->Read(overlay);

		MapOverlay &current = fOverlays[index];
		current.fTileSet = overlay.resource_ref;
		current.fWidth = overlay.width;
		current.fHeight = overlay.height;
		current.fTileMaps = new TileMap[overlay.width * overlay.height];

		const uint32 overlaySize = overlay.height * overlay.width;
		for (uint32 x = 0; x < overlaySize; x++) {
			uint32 mapOffset = overlay.tilemap_offset + x * sizeof(tilemap);
			tilemap tileMap;
			fData->ReadAt(mapOffset, tileMap);

			current.fTileMaps[x].SetMask(tileMap.mask);

			const int32 indexCount = tileMap.primary_tile_count;
			const int32 offset = overlay.tile_lookup_offset
					+ (tileMap.primary_tile_index * sizeof(int16));

			int16 tisIndex;
			for (int32 c = 0; c < indexCount; c++) {
				fData->ReadAt(offset + c * sizeof(int16), tisIndex);
				current.fTileMaps[x].AddTileIndex(tisIndex);
			}

			const int32 secondaryOffset = overlay.tile_lookup_offset
					+ (tileMap.secondary_tile_index * sizeof(int16));

			fData->ReadAt(secondaryOffset, tisIndex);
			current.fTileMaps[x].SetSecondaryTileIndex(tisIndex);
		}
	}
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

	for (int32 p = 0; p < fNumPolygons; p++) {
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


TileMap *
MapOverlay::TileMapForTile(int32 i)
{
	// TODO: handle this more correctly
	if (i >= fWidth * fHeight)
		i = (fWidth * fHeight) - 1;

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


TileMap::TileMap()
	:
	fSecondaryIndex(0)
{
}


void
TileMap::AddTileIndex(uint16 index)
{
	fIndices.push_back(index);
}


uint16
TileMap::TileIndex()
{
	uint16 index = fIndices[fCurrentIndex];
	if (++fCurrentIndex >= fIndices.size())
		fCurrentIndex = 0;
	return index;
}


void
TileMap::SetSecondaryTileIndex(uint16 index)
{
	fSecondaryIndex = index;
}


uint16
TileMap::SecondaryTileIndex() const
{
	return fSecondaryIndex;
}


void
TileMap::SetMask(uint8 mask)
{
	fMask = mask;
}


uint8
TileMap::Mask() const
{
	return fMask;
}
