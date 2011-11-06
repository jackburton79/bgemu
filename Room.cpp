#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "Graphics.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Room.h"
#include "TisResource.h"
#include "WedResource.h"

#include <iostream>

Room::Room()
	:
	fSurface(NULL),
	fArea(NULL),
	fLightMap(NULL),
	fSearchMap(NULL),
	fHeightMap(NULL),
	fAnimations(NULL),
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
	SDL_FreeSurface(fSurface);
	SDL_FreeSurface(fLightMap);
	SDL_FreeSurface(fSearchMap);
	SDL_FreeSurface(fHeightMap);
	delete[] fAnimations;
	gResManager->ReleaseResource(fArea);
}


SDL_Rect
Room::Rect() const
{
	SDL_Rect rect = { 0, 0, fSurface->w, fSurface->h };
	return rect;
}


bool
Room::Load(const char *resName)
{
	std::cout << "RegionMap::Load(" << resName << ")" << std::endl;

	fArea = gResManager->GetAREA(resName);
	fName = fArea->WedName();

	WEDResource *wed = gResManager->GetWED(fName);
	fSurface = wed->GetAreaMap();
	gResManager->ReleaseResource(wed);

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

	return true;
}


void
Room::Draw(SDL_Surface *surface, SDL_Rect area)
{
	SDL_BlitSurface(fSurface, &area, surface, NULL);

	if (fDrawAnimations)
		_DrawAnimations(surface, area);

	if (fDrawLightMap)
		_DrawLightMap(surface);
	if (fDrawHeightMap)
		_DrawHeightMap(surface, area);
	if (fDrawSearchMap)
		_DrawSearchMap(surface, area);

	if (fDrawPolygons) {
		// TODO:
	}

	SDL_Flip(surface);
}


void
Room::ToggleOverlays()
{
	fDrawOverlays = !fDrawOverlays;
	SDL_FreeSurface(fSurface);

	WEDResource *wed = gResManager->GetWED(fName);
	fSurface = wed->GetAreaMap(fDrawOverlays);
	gResManager->ReleaseResource(wed);
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

	SDL_Rect fill = {
			destRect.x + (area.x / (fSurface->w / fSearchMap->w)),
			destRect.y + (area.y / (fSurface->h / fSearchMap->h)),
			(area.w / (fSurface->w / fSearchMap->w)),
			(area.h / (fSurface->h / fSearchMap->h))
	};
	SDL_FillRect(surface, &fill, 45);
}


void
Room::_DrawHeightMap(SDL_Surface *surface, SDL_Rect area)
{
	SDL_Rect destRect = {
		fLightMap->w + fSearchMap->w, surface->h - fHeightMap->h,
		fSearchMap->w + fLightMap->w + fHeightMap->w, fHeightMap->h
	};
	SDL_BlitSurface(fHeightMap, NULL, surface, &destRect);

	SDL_Rect fill = {
		destRect.x + (area.x / (fSurface->w / fHeightMap->w)),
		destRect.y + (area.y / (fSurface->h / fHeightMap->h)),
		(area.w / (fSurface->w / fHeightMap->w)),
		(area.h / (fSurface->h / fHeightMap->h))
	};
	SDL_FillRect(surface, &fill, 45);
}


void
Room::_DrawAnimations(SDL_Surface *surface, SDL_Rect area)
{
	if (fAnimations == NULL)
		return;

	for (int32 i = 0; i < fArea->CountAnimations(); i++) {
		if (fAnimations[i] != NULL) {
			point center = fAnimations[i]->fCenter;
			Frame frame = fAnimations[i]->Image();
			SDL_Surface *animImage = frame.surface;
			if (animImage == NULL)
				continue;
			SDL_Rect rect = {
					center.x - frame.rect.w / 2,
					center.y - frame.rect.h,
					animImage->w, animImage->h
			};

			//if (!rects_intersect(area, rect))
				//continue;
			/*SDL_Rect rect = {
					0, 0, animImage->w, animImage->h
			};*/
			//printf("%d %d %d %d\n", rect.x, rect.y,
				//	rect.w, rect.h);
			//rect = offset_rect(rect, -frame.rect.x, 0);
			rect = offset_rect(rect, -area.x, -area.y);
			SDL_BlitSurface(animImage, NULL, surface, &rect);
			SDL_FreeSurface(animImage);
		}
	}
}


void
Room::_InitAnimations()
{
	fAnimations = new Animation*[fArea->CountAnimations()];
	for (int32 i = 0; i < fArea->CountAnimations(); i++) {
		fAnimations[i] = new Animation(fArea->AnimationAt(i));
	}
}


// Animation
Animation::Animation(animation *animDesc)
	:
	fBAM(NULL),
	fCycle(NULL),
	fNumber(0),
	fCurrentFrame(0)
{
	fBAM = gResManager->GetBAM(animDesc->bam_name);
	fNumber = animDesc->sequence;
	fCycle = fBAM->CycleAt(fNumber);
	fCenter = animDesc->center;
	fCurrentFrame = animDesc->frame;
	fHold = animDesc->flags & ANIM_HOLD;

	printf("animation %s\n", (const char*)animDesc->bam_name);
	printf("\tsequence %d%s\n", fNumber,
			fHold ? ", still frame": "");
	printf("\t%d frames\n", fCycle->numFrames);
	printf("\tcenter: %d %d\n", fCenter.x, fCenter.y);
}


Animation::~Animation()
{
	gResManager->ReleaseResource(fBAM);
	delete fCycle;
}


Frame
Animation::Image()
{
	//printf("center: %d %d\n", fCenter.x, fCenter.y);

	Frame frame = fBAM->FrameForCycle(fCurrentFrame, fCycle);
	//printf("frame center: %d %d\n", frame.rect.x, frame.rect.y);
	if (!fHold) {
		fCurrentFrame++;
		if (fCurrentFrame >= fCycle->numFrames)
			fCurrentFrame = 0;
	}
	return frame;
}
