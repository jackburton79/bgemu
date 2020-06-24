#include "Bitmap.h"
#include "TisResource.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "Stream.h"

#include <iostream>

#define TIS_SIGNATURE "TIS "
#define TIS_VERSION_1 "V1.0"

const static int kTileDataSize = 1024 + 4096;
const static int kDataOffset = 8; //strlen(TIS_SIGNATURE) + strlen(TIS_VERSION_1);

/* static */
Resource*
TISResource::Create(const res_ref& name)
{
	return new TISResource(name);
}


TISResource::TISResource(const res_ref &name)
	:
	Resource(name, RES_TIS),
	fDataOffset(0)
{
}


TISResource::~TISResource()
{
	std::map<int, Bitmap*>::iterator i;
	for (i = fCachedTiles.begin(); i != fCachedTiles.end(); i++) {
		i->second->Release();
	}
}


bool
TISResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (CheckSignature(TIS_SIGNATURE, true)) {
		if (CheckVersion(TIS_VERSION_1, true))
			fDataOffset = kDataOffset;
	}
	return true;
}


Bitmap*
TISResource::TileAt(int index)
{
	if (index < 0)
		return NULL;

	Bitmap* tile = NULL;
	std::map<int, Bitmap*>::iterator i = fCachedTiles.find(index);
	if (i != fCachedTiles.end())
		tile = i->second;
	else {
		tile = _GetTileAt(index);
		fCachedTiles[index] = tile;
	}

	if (tile != NULL)
		tile->Acquire();

	return tile;
}


Bitmap*
TISResource::_GetTileAt(int index)
{
	fData->Seek(fDataOffset + index * kTileDataSize, SEEK_SET);

	Bitmap* bitmap = new Bitmap(TILE_WIDTH, TILE_HEIGHT, 8);
	try {
		GFX::Palette palette;
		for (int i = 0; i < 256; i++) {
			palette.colors[i].b = fData->ReadByte();
			palette.colors[i].g = fData->ReadByte();
			palette.colors[i].r = fData->ReadByte();
			palette.colors[i].a = fData->ReadByte();
		}

		uint8 *pixels = reinterpret_cast<uint8*>(bitmap->Pixels());
		for (int i = 0; i < 4096; i++) {
			uint8 pixel = fData->ReadByte();
			*pixels++ = pixel;
		}

		bitmap->SetPalette(palette);
	} catch (...) {
		bitmap->Release();
		bitmap = NULL;
	}

	return bitmap;
}
