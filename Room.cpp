#include "Actor.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "Graphics.h"
#include "IDSResource.h"
#include "RectUtils.h"
#include "ResManager.h"
#include "Room.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "WedResource.h"
#include "World.h"

#include <iostream>

Room::Room()
	:
	fSurface(NULL),
	fArea(NULL),
	fLightMap(NULL),
	fSearchMap(NULL),
	fHeightMap(NULL),
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
	SDL_FreeSurface(fSurface);
	SDL_FreeSurface(fLightMap);
	SDL_FreeSurface(fSearchMap);
	SDL_FreeSurface(fHeightMap);
	delete[] fAnimations;
	delete[] fActors;
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

	fArea = gResManager->GetARA(resName);
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
	_InitActors();

	return true;
}


void
Room::Draw(SDL_Surface *surface, SDL_Rect area)
{
	SDL_BlitSurface(fSurface, &area, surface, NULL);

	if (fDrawAnimations)
		_DrawAnimations(surface, area);

	if (true)
		_DrawActors(surface, area);

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
	fSurface = wed->GetAreaMap(fDrawOverlays, fDrawPolygons);
	gResManager->ReleaseResource(wed);
}


void
Room::TogglePolygons()
{
	fDrawPolygons = !fDrawPolygons;
	SDL_FreeSurface(fSurface);

	WEDResource *wed = gResManager->GetWED(fName);
	fSurface = wed->GetAreaMap(fDrawOverlays, fDrawPolygons);
	gResManager->ReleaseResource(wed);
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
			Frame frame = fAnimations[i]->Image();
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

	// TODO: Get the correct bam for actor
	for (uint16 a = 0; a < fArea->CountActors(); a++) {
		//CREResource *cre = fActors[a]->CRE();

		//uint16 id = cre->AnimationID();
		//uint32 nameID = cre->ShortNameID();
		//printf("race %d, class %d, gender %d\n",
			//	cre->Race(), cre->Class(), cre->Gender());

		res_ref resRef = Actor::AnimationFor(*fActors[a]);

		BAMResource *bam = gResManager->GetBAM(resRef);
		::cycle *cycle = bam->CycleAt(0);
		Frame frame = bam->FrameForCycle(0, cycle);
		delete cycle;
		SDL_Surface *image = frame.surface;
		point center = fActors[a]->Position();
		gResManager->ReleaseResource(bam);

		if (image == NULL)
			continue;

		center = offset_point(center, -frame.rect.w / 2,
						-frame.rect.h / 2);

		SDL_Rect rect = { center.x, center.y,
				image->w, image->h };

		rect = offset_rect(rect, -frame.rect.x, -frame.rect.y);

		if (!rects_intersect(area, rect))
				continue;

		rect = offset_rect(rect, -area.x, -area.y);
		SDL_BlitSurface(image, NULL, surface, &rect);
		SDL_FreeSurface(image);

		//const char *string = fIDSAnimate->ValueFor(id);
		//printf("string: %s\n", string);
		/*TLKEntry *entry = Dialogs()->EntryAt(nameID);
		if (entry == NULL)
			continue;
		printf("actor %s (%d): %s\n", fActors[a]->Name(),
				id, entry->string);*/
		//delete entry;
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


void
Room::_InitActors()
{
	fActors = new Actor*[fArea->CountActors()];
	for (uint16 i = 0; i < fArea->CountActors(); i++) {
		fActors[i] = new Actor(*fArea->ActorAt(i));
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
}


Animation::~Animation()
{
	gResManager->ReleaseResource(fBAM);
	delete fCycle;
}


Frame
Animation::Image()
{
	Frame frame = fBAM->FrameForCycle(fCurrentFrame, fCycle);
	if (!fHold) {
		fCurrentFrame++;
		if (fCurrentFrame >= fCycle->numFrames)
			fCurrentFrame = 0;
	}
	return frame;
}
