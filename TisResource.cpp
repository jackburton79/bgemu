#include "TisResource.h"
#include "MemoryStream.h"
#include "Stream.h"

#include <iostream>

#define TIS_SIGNATURE "TIS "
#define V1 ""

const static int kTileDataSize = 1024 + 4096;

/*
TISResource::TISResource(uint8 *data, uint32 size, uint32 key)
	:
	Resource(data, size, key)
{
	fType = RES_TIS;
}
*/

TISResource::TISResource(const res_ref &name)
	:
	Resource(name, RES_TIS)
{
}


TISResource::~TISResource()
{
}


bool
TISResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	// TODO: No signature ? It seems these resources
	// start directly with data
	/*char signature[5];
	signature[4] = '\0';
	fData->ReadAt(0, signature, 4);

	if (strcmp(signature, TIS_SIGNATURE)) {
		printf("invalid signature %s\n", signature);
		return false;
	}*/

	return true;
}


SDL_Surface *
TISResource::TileCellAt(int index)
{
	if (index < 0)
		throw -1;

	fData->Seek(index * kTileDataSize, SEEK_SET);
	
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 64, 64, 8, 0, 0, 0, 0);
	
	SDL_Color palette[256];
	for (int32 i = 0; i < 256; i++) {
		palette[i].b = fData->ReadByte();
		palette[i].g = fData->ReadByte();
		palette[i].r = fData->ReadByte();
		palette[i].unused = fData->ReadByte();
	}
	
	SDL_LockSurface(surface);
	uint8 *pixels = (uint8 *)surface->pixels;
	for (int i = 0; i < 4096; i++) {
		uint8 pixel = fData->ReadByte();
		*pixels++ = pixel;	
	}
	
	SDL_UnlockSurface(surface);
	SDL_SetColors(surface, palette, 0, 256);

	return surface;
}
