#ifndef __REGION_MAP_H
#define __REGION_MAP_H

#include "SDL.h"
#include "TileCell.h"
#include "Types.h"

class Actor;
class Animation;
class ARAResource;
class Door;
class Room {
public:
	Room();
	~Room();
	
	SDL_Rect Rect() const;

	bool Load(const char *name);
	void Draw(SDL_Surface *surface, SDL_Rect area);
	void Clicked(uint16 x, uint16 y);

	uint16 TileNumberForPoint(uint16 x, uint16 y);

	void ToggleOverlays();
	void TogglePolygons();
	void ToggleAnimations();
	void ToggleLightMap();
	void ToggleSearchMap();
	void ToggleHeightMap();

private:
	void _DrawBaseMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawLightMap(SDL_Surface *surface);
	void _DrawSearchMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawHeightMap(SDL_Surface *surface, SDL_Rect area);
	void _DrawAnimations(SDL_Surface *surface, SDL_Rect area);
	void _DrawActors(SDL_Surface *surface, SDL_Rect area);

	void _InitAnimations();
	void _InitActors();
	void _InitDoors();

	res_ref fName;
	SDL_Rect fVisibleArea;

	WEDResource *fWed;
	ARAResource *fArea;

	SDL_Surface *fLightMap;
	SDL_Surface *fSearchMap;
	SDL_Surface *fHeightMap;

	Animation **fAnimations;
	Actor **fActors;
	Door **fDoors;

	bool fDrawOverlays;
	bool fDrawPolygons;
	bool fDrawLightMap;
	bool fDrawSearchMap;
	bool fDrawHeightMap;
	bool fDrawAnimations;

};

#endif
