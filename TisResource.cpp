#include "TisResource.h"
#include "MemoryStream.h"
#include "Stream.h"

#include <iostream>

#define TIS_SIGNATURE "TIS "
#define TIS_VERSION_1 "V1.0"

const static int kTileDataSize = 1024 + 4096;

TISResource::TISResource(const res_ref &name)
	:
	Resource(name, RES_TIS),
	fDataOffset(0)
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

	if (CheckSignature(TIS_SIGNATURE, true)) {
		if (CheckVersion(TIS_VERSION_1, true))
			fDataOffset = strlen(TIS_SIGNATURE) + strlen(TIS_VERSION_1);
	}
	return true;
}


SDL_Surface *
TISResource::TileAt(int index)
{
	if (index < 0)
		return NULL;

	fData->Seek(fDataOffset + index * kTileDataSize, SEEK_SET);
	
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			TILE_WIDTH, TILE_HEIGHT, 8, 0, 0, 0, 0);
	
	try {
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

	} catch (...) {
		SDL_FreeSurface(surface);
		return NULL;
	}
	

	return surface;
}
