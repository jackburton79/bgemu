#include "TisResource.h"
#include "MemoryStream.h"
#include "Stream.h"

#include <iostream>

TISResource::TISResource(uint8 *data, uint32 size, uint32 key)
	:
	Resource(data, size, key)
{
	fType = RES_TIS;
}


TISResource::TISResource(const res_ref &name)
	:
	Resource()
{
	fType = RES_TIS;
}


TISResource::~TISResource()
{
}


bool
TISResource::Load(TArchive *archive, uint32 key)
{
	return Resource::Load(archive, key);
}


SDL_Surface *
TISResource::TileCellAt(int index)
{
	if (index < 0)
		throw -1;

	fData->Seek(index * (1024 + 4096), SEEK_SET);
	
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 64, 64, 8, 0, 0, 0, 0);
	
	SDL_Color Palette[256];
	for (int32 i = 0; i < 256; i++) {
		Palette[i].b = fData->ReadByte();
		Palette[i].g = fData->ReadByte();
		Palette[i].r = fData->ReadByte();
		Palette[i].unused = fData->ReadByte();
	}
	
	SDL_LockSurface(surface);
	uint8 *pixels = (uint8 *)surface->pixels;
	for (int i = 0; i < 4096; i++) {
		uint8 pixel = fData->ReadByte();
		*pixels++ = pixel;	
	}
	
	SDL_UnlockSurface(surface);
	SDL_SetColors(surface, Palette, 0, 256);

	return surface;
}
