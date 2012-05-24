#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "SDL.h"
#include "IETypes.h"

#include <vector>

class Actor;
class Animation;
class ARAResource;
class Door;
class MapOverlay;
class Script;
class TileCell;
class Room {
public:
	Room();
	~Room();
	
	bool Load(const char *name);

	SDL_Rect ViewPort() const;
	void SetViewPort(SDL_Rect rect);

	void Draw(SDL_Surface *surface);
	void Clicked(uint16 x, uint16 y);
	void MouseOver(uint16 x, uint16 y);
	uint16 TileNumberForPoint(uint16 x, uint16 y);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleLightMap();
	void ToggleSearchMap();
	void ToggleHeightMap();

	void DrawTile(const int16 tileNum, SDL_Surface *surface,
						SDL_Rect tileRect, bool withOverlays);

	void DumpOverlays(const char *path);

private:
	void _DrawBaseMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawOverlay(SDL_Surface *surface, SDL_Surface *cell,
				SDL_Rect rect, SDL_Color *transparent);

	void _DrawLightMap(SDL_Surface *surface);
	void _DrawSearchMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawHeightMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawAnimations(SDL_Surface *surface, SDL_Rect area);
	void _DrawActors(SDL_Surface *surface, SDL_Rect area);

	void _LoadOverlays();
	void _InitVariables();
	void _InitTileCells();
	void _InitAnimations();
	void _InitActors();
	void _InitDoors();

	res_ref fName;
	SDL_Rect fVisibleArea;

	WEDResource *fWed;
	ARAResource *fArea;
	BCSResource *fBcs;

	SDL_Surface *fLightMap;
	SDL_Surface *fSearchMap;
	SDL_Surface *fHeightMap;

	std::vector<MapOverlay*> fOverlays;
	std::vector<TileCell*> fTileCells;

	std::vector<Animation*> fAnimations;
	std::vector<Actor*> fActors;
	std::vector<Door*> fDoors;

	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawLightMap;
	bool fDrawSearchMap;
	bool fDrawHeightMap;
	bool fDrawAnimations;

};


#endif
