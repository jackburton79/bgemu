#include "Actor.h"
#include "Animation.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "Door.h"
#include "Graphics.h"
#include "IDSResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Room.h"
#include "Script.h"
#include "Tile.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "World.h"

#include <assert.h>
#include <iostream>

Room::Room()
	:
	fWed(NULL),
	fArea(NULL),
	fLightMap(NULL),
	fSearchMap(NULL),
	fHeightMap(NULL),
	fNumOverlays(0),
	fOverlays(NULL),
	fTiles(NULL),
	fAnimations(NULL),
	fActors(NULL),
	fDrawOverlays(false),
	fDrawPolygons(false),
	fDrawLightMap(false),
	fDrawSearchMap(false),
	fDrawHeightMap(false),
	fDrawAnimations(false)
{
}


Room::~Room()
{
	SDL_FreeSurface(fLightMap);
	SDL_FreeSurface(fSearchMap);
	SDL_FreeSurface(fHeightMap);
	delete[] fOverlays;
	delete[] fTiles;
	delete[] fAnimations;
	delete[] fActors;
	gResManager->ReleaseResource(fWed);
	gResManager->ReleaseResource(fArea);
}


SDL_Rect
Room::ViewPort() const
{
	return fVisibleArea;
}


bool
Room::Load(const char *resName)
{
	std::cout << "Room::Load(" << resName << ")" << std::endl;

	fArea = gResManager->GetARA(resName);
	fName = fArea->WedName();
	fWed = gResManager->GetWED(fName);

	BCSResource *bcs = gResManager->GetBCS(fArea->Script());
	fScript = bcs->GetScript();
	//gResManager->ReleaseResource(bcs);

	_LoadOverlays();
	_LoadTiles();

	BMPResource *bmp = gResManager->GetBMP(
			ResourceManager::HeightMapName(fName).c_str());
	fHeightMap = bmp->Image();
	gResManager->ReleaseResource(bmp);

	bmp = gResManager->GetBMP(
			ResourceManager::LightMapName(fName).c_str());
	fLightMap = bmp->Image();

	gResManager->ReleaseResource(bmp);

	bmp = gResManager->GetBMP(
			ResourceManager::SearchMapName(fName).c_str());
	fSearchMap = bmp->Image();
	gResManager->ReleaseResource(bmp);

	_InitAnimations();
	_InitActors();
	_InitDoors();

	// Execute room script
	// TODO: Move away from here.
	// TODO: Is it correct here ?
	//_ExecuteScript(fScript);

	return true;
}


void
Room::SetViewPort(SDL_Rect rect)
{
	const uint16 areaWidth = fOverlays[0]->Width() * TILE_WIDTH;
	const uint16 areaHeigth = fOverlays[0]->Height() * TILE_HEIGHT;

	if (rect.x < 0)
		rect.x = 0;
	if (rect.y < 0)
		rect.y = 0;
	if (rect.x + rect.w > areaWidth)
		rect.x = areaWidth - rect.w;
	if (rect.y + rect.h > areaHeigth)
		rect.y = areaHeigth - rect.h;

	rect.x = std::max(rect.x, (Sint16)0);
	rect.y = std::max(rect.y, (Sint16)0);

	fVisibleArea = rect;
}


void
Room::Draw(SDL_Surface *surface)
{
	//fVisibleArea = area;

	_DrawBaseMap(surface, fVisibleArea);

	if (fDrawAnimations)
		_DrawAnimations(surface, fVisibleArea);

	if (true)
		_DrawActors(surface, fVisibleArea);

	if (fDrawLightMap)
		_DrawLightMap(surface);
	if (fDrawHeightMap)
		_DrawHeightMap(surface, fVisibleArea);
	if (fDrawSearchMap)
		_DrawSearchMap(surface, fVisibleArea);

	if (fDrawPolygons) {
		// TODO:
	}

	SDL_Flip(surface);
}


void
Room::Clicked(uint16 x, uint16 y)
{
	x += fVisibleArea.x;
	y += fVisibleArea.y;

	const uint16 tileNum = TileNumberForPoint(x, y);
	printf("tileNum: %d\n", tileNum);
	fTiles[tileNum]->Clicked();
}


void
Room::MouseOver(uint16 x, uint16 y)
{
	x += fVisibleArea.x;
	y += fVisibleArea.y;
	const uint16 tileNum = TileNumberForPoint(x, y);
	fTiles[tileNum]->MouseOver();
}


uint16
Room::TileNumberForPoint(uint16 x, uint16 y)
{
	const uint16 overlayWidth = fOverlays[0]->Width();
	const uint16 tileX = x / TILE_WIDTH;
	const uint16 tileY = y / TILE_HEIGHT;

	return tileY * overlayWidth + tileX;
}


void
Room::ToggleOverlays()
{
	fDrawOverlays = !fDrawOverlays;
	SDL_Flip(SDL_GetVideoSurface());
}


void
Room::TogglePolygons()
{
	fDrawPolygons = !fDrawPolygons;
}


void
Room::ToggleLightMap()
{
	fDrawLightMap = !fDrawLightMap;
}


void
Room::ToggleSearchMap()
{
	fDrawSearchMap = !fDrawSearchMap;
}


void
Room::ToggleHeightMap()
{
	fDrawHeightMap = !fDrawHeightMap;
}


void
Room::ToggleAnimations()
{
	fDrawAnimations = !fDrawAnimations;
}


void
Room::_DrawBaseMap(SDL_Surface *surface, SDL_Rect area)
{
	MapOverlay *overlay = fOverlays[0];
	const uint16 overlayWidth = overlay->Width();
	const uint16 firstTileX = area.x / TILE_WIDTH;
	const uint16 firstTileY = area.y / TILE_HEIGHT;
	uint16 lastTileX = 1 + (area.x + area.w) / TILE_WIDTH;
	uint16 lastTileY = 1 + (area.y + area.h) / TILE_HEIGHT;

	lastTileX = std::min(lastTileX, overlayWidth);
	lastTileY = std::min(lastTileY, overlay->Height());

	SDL_Rect tileRect = { 0, 0, TILE_WIDTH, TILE_HEIGHT };
	for (uint16 y = firstTileY; y < lastTileY; y++) {
		tileRect.y = y * TILE_HEIGHT;
		for (uint16 x = firstTileX; x < lastTileX; x++) {
			tileRect.x = x * TILE_WIDTH;
			const uint32 tileNum = y * overlayWidth + x;
			SDL_Rect rect = offset_rect(tileRect, -area.x, -area.y);
			fTiles[tileNum]->Draw(surface, &rect, fDrawOverlays);
		}
	}
}


void
Room::_DrawLightMap(SDL_Surface *surface)
{
	SDL_Rect destRect = {
		0, surface->h - fLightMap->h,
		fLightMap->w, fLightMap->h
	};
	SDL_BlitSurface(fLightMap, NULL, surface, &destRect);
}


void
Room::_DrawSearchMap(SDL_Surface *surface, SDL_Rect area)
{
	SDL_Rect destRect = {
		fLightMap->w, surface->h - fSearchMap->h,
		fSearchMap->w + fLightMap->w, fSearchMap->h
	};
	SDL_BlitSurface(fSearchMap, NULL, surface, &destRect);
}


void
Room::_DrawHeightMap(SDL_Surface *surface, SDL_Rect area)
{
	SDL_Rect destRect = {
		fLightMap->w + fSearchMap->w, surface->h - fHeightMap->h,
		fSearchMap->w + fLightMap->w + fHeightMap->w, fHeightMap->h
	};
	SDL_BlitSurface(fHeightMap, NULL, surface, &destRect);

	/*MapOverlay *overlay = fWed->OverlayAt(0);
	SDL_Rect fill = {
		destRect.x + (area.x / (overlay->Width() / fHeightMap->w)),
		destRect.y + (area.y / (overlay->Height() / fHeightMap->h)),
		(area.w / (overlay->Width() / fHeightMap->w)),
		(area.h / (overlay->Height() / fHeightMap->h))
	};
	SDL_FillRect(surface, &fill, 45);*/
}


void
Room::_DrawAnimations(SDL_Surface *surface, SDL_Rect area)
{
	if (fAnimations == NULL)
		return;

	for (uint32 i = 0; i < fArea->CountAnimations(); i++) {
		if (fAnimations[i] != NULL) {
			Frame frame = fAnimations[i]->NextFrame();
			point center = fAnimations[i]->fCenter;
			center = offset_point(center, -frame.rect.w / 2,
					-frame.rect.h / 2);
			SDL_Surface *animImage = frame.surface;
			if (animImage == NULL)
				continue;

			SDL_Rect rect = { center.x, center.y,
					animImage->w, animImage->h };

			rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

			if (!rects_intersect(area, rect))
				continue;

			rect = offset_rect(rect, -area.x, -area.y);

			SDL_BlitSurface(animImage, NULL, surface, &rect);
			SDL_FreeSurface(animImage);
		}
	}
}


void
Room::_DrawActors(SDL_Surface *surface, SDL_Rect area)
{
	if (fActors == NULL)
		return;

	for (uint16 a = 0; a < fArea->CountActors(); a++) {
		try {
			fActors[a]->Draw(surface, area);
		} catch (...) {
			continue;
		}
	}
}


void
Room::_LoadOverlays()
{
	fNumOverlays = fWed->CountOverlays();
	fOverlays = new MapOverlay*[fNumOverlays];
	for (uint32 i = 0; i < fNumOverlays; i++)
		fOverlays[i] = fWed->GetOverlay(i);

}


void
Room::_LoadTiles()
{
	uint32 numTiles = fOverlays[0]->Size();
	fTiles = new Tile*[numTiles];
	for (uint16 i = 0; i < numTiles; i++) {
		fTiles[i] = new Tile(i);
		for (uint32 o = 0; o < fNumOverlays; o++)
			fTiles[i]->AddTileMap(fOverlays[o]->TileMapForTile(i));
	}
}


void
Room::_InitAnimations()
{
	fAnimations = new Animation*[fArea->CountAnimations()];
	for (uint32 i = 0; i < fArea->CountAnimations(); i++) {
		fAnimations[i] = new Animation(fArea->AnimationAt(i));
	}
}


void
Room::_InitActors()
{
	fActors = new Actor*[fArea->CountActors()];
	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		fActors[i] = new Actor(*fArea->ActorAt(i));
	}
}


void
Room::_InitDoors()
{
	assert(fTiles != NULL);

	uint32 numDoors = fWed->CountDoors();
	fDoors = new Door*[numDoors];
	for (uint32 c = 0; c < numDoors; c++) {
		Door *door = fWed->GetDoor(c);
		fDoors[c] = door;
		for (uint32 i = 0; i < door->fTilesOpen.size(); i++) {
			fTiles[door->fTilesOpen[i]]->SetDoor(door);
		}
	}
}


void
Room::_ExecuteScript(Script *script)
{
	// for each CR block
	// for each CO block
	// check triggers
	// if any trigger
	// find response_set block
	// choose response
	// execute actions

	::node* root = script->RootNode();
	bool evaluation = true;
	::node* condRes = fScript->FindNode(BLOCK_CONDITION_RESPONSE, root);
	do {
		::node* cond = fScript->FindNode(BLOCK_CONDITION, condRes);
		::node* responseSet = cond->Next();
		for (;;) {
			if (cond == NULL)
				break;

			node* trig = fScript->FindNode(BLOCK_TRIGGER, cond);
			for (;;) {
				if (trig == NULL)
					break;
				evaluation = _EvaluateTrigger(static_cast<trigger*>(trig)) && evaluation;
				if (!evaluation)
					break;
				trig = trig->Next();
			}
			// If no trigger were true, don't consider responses
			if (!evaluation)
				break;

			responseSet = cond->Next();
			if (responseSet == NULL)
				break;
			node* act = fScript->FindNode(BLOCK_ACTION, responseSet);
			for (;;) {
				if (act == NULL)
					break;

				_ExecuteAction(static_cast<action*>(act));

				act = act->Next();
			}

			cond = responseSet->Next();
		}
	} while ((condRes = condRes->Next()) != NULL);



}


bool
Room::_EvaluateTrigger(trigger* trig)
{
	trig->Print();
	printf("%s\n", TriggerIDS()->ValueFor(trig->id));
	return true;
}


void
Room::_ExecuteAction(action* act)
{
	act->Print();
	printf("%s\n", ActionIDS()->ValueFor(act->id));
}
