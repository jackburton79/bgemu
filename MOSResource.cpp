/*
 * MOSResource.cpp
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "MOSResource.h"

#include <zlib.h>


#define MOS_SIGNATURE "MOS "
#define MOSC_SIGNATURE "MOSC"
#define MOS_VERSION_1 "V1  "

const static int kPaletteDataSize = 1024;

MOSResource::MOSResource(const res_ref &name)
	:
	Resource(name, RES_MOS)
{
}


MOSResource::~MOSResource()
{
	// TODO Auto-generated destructor stub
}


/* virtual */
bool
MOSResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (CheckSignature(MOSC_SIGNATURE)) {
		if (!CheckVersion(MOS_VERSION_1))
			return false;

		uint32 len;
		fData->ReadAt(8, len);
		uint8 *decompressedData = new uint8[len];
		int status = uncompress((Bytef*)decompressedData,
			(uLongf*)&len, (const Bytef*)(fData->Data()) + 12, fData->Size() - 12);

		if (status != Z_OK) {
			delete[] decompressedData;
			return false;
		}

		ReplaceData(new MemoryStream(decompressedData, len, true));
	}


	if (!CheckSignature(MOS_SIGNATURE))
		return false;

	if (!CheckVersion(MOS_VERSION_1))
		return false;

	fData->ReadAt(8, fWidth);
	fData->ReadAt(10, fHeight);
	fData->ReadAt(12, fColumns);
	fData->ReadAt(14, fRows);
	fData->ReadAt(16, fBlockSize);
	fData->ReadAt(20, fPaletteOffset);

	fTileOffsets = fPaletteOffset + kPaletteDataSize * fColumns * fRows;
	fPixelDataOffset = fTileOffsets + fColumns * fRows * sizeof(uint32);

	printf("width: %d, height: %d, columns: %d, rows: %d, block: %d\n",
			fWidth, fHeight, fColumns, fRows, fBlockSize);
	return true;
}


Bitmap*
MOSResource::Image()
{
	GFX::rect tileRect = { 0, 0, fBlockSize, fBlockSize };
	Bitmap* bitmap = GraphicsEngine::CreateBitmap(fWidth, fHeight, 16);
	for (uint16 y = 0; y < fRows - 1; y++) {
		tileRect.y = y * fBlockSize;
		for (uint16 x = 0; x < fColumns - 1; x++) {
			tileRect.x = x * fBlockSize;
			const uint32 tileNum = y * fColumns + x;
			Bitmap* tile = TileAt(tileNum);
			if (tile == NULL) {
				printf("NULL tile\n");
			}
			GraphicsEngine::BlitBitmap(tile, NULL, bitmap, &tileRect);
			GraphicsEngine::DeleteBitmap(tile);
		}
	}
	return bitmap;
}


Bitmap*
MOSResource::TileAt(int index)
{
	if (index < 0)
		return NULL;

	fData->Seek(fPaletteOffset + index * kPaletteDataSize, SEEK_SET);

	Bitmap* surface = GraphicsEngine::CreateBitmap(
			fBlockSize, fBlockSize, 8);
	try {
		Palette palette;
		for (int32 i = 0; i < 256; i++) {
			palette.colors[i].b = fData->ReadByte();
			palette.colors[i].g = fData->ReadByte();
			palette.colors[i].r = fData->ReadByte();
			palette.colors[i].a = fData->ReadByte();
		}

		fData->Seek(fTileOffsets + index * sizeof(uint32), SEEK_SET);

		uint32 tileOffset;
		fData->Read(tileOffset);
		fData->Seek(fPixelDataOffset + tileOffset, SEEK_SET);

		int tileDataSize = fBlockSize * fBlockSize;
		uint8 *pixels = (uint8 *)surface->Pixels();
		for (int i = 0; i < tileDataSize; i++) {
			uint8 pixel = fData->ReadByte();
			*pixels++ = pixel;
		}

		surface->SetPalette(palette);

	} catch (...) {
		printf("Caught exception\n");
		GraphicsEngine::DeleteBitmap(surface);
		return NULL;
	}

	return surface;
}
