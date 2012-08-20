#include "Bitmap.h"
#include "TisResource.h"
#include "GraphicsEngine.h"
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


Bitmap*
TISResource::TileAt(int index)
{
	if (index < 0)
		return NULL;

	fData->Seek(fDataOffset + index * kTileDataSize, SEEK_SET);
	
	Bitmap* surface = GraphicsEngine::CreateBitmap(
			TILE_WIDTH, TILE_HEIGHT, 8);
	
	try {
		Palette palette;
		for (int32 i = 0; i < 256; i++) {
			palette.colors[i].b = fData->ReadByte();
			palette.colors[i].g = fData->ReadByte();
			palette.colors[i].r = fData->ReadByte();
			palette.colors[i].a = fData->ReadByte();
		}

		uint8 *pixels = (uint8 *)surface->Pixels();
		for (int i = 0; i < 4096; i++) {
			uint8 pixel = fData->ReadByte();
			*pixels++ = pixel;
		}

		surface->SetPalette(palette);

	} catch (...) {
		GraphicsEngine::DeleteBitmap(surface);
		return NULL;
	}
	

	return surface;
}
