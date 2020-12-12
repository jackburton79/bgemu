/*
 * MOSResource.cpp
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */

#include "MOSResource.h"

#include "Bitmap.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "ZLibDecompressor.h"

#define MOS_SIGNATURE "MOS "
#define MOSC_SIGNATURE "MOSC"
#define MOS_VERSION_1 "V1  "

const static int kPaletteDataSize = 1024;

/* static */
Resource*
MOSResource::Create(const res_ref& name)
{
	return new MOSResource(name);
}


MOSResource::MOSResource(const res_ref& name)
	:
	Resource(name, RES_MOS),
	fWidth(0),
	fHeight(0),
	fColumns(0),
	fRows(0),
	fBlockSize(0),
	fPaletteOffset(0),
	fTileOffsets(0),
	fPixelDataOffset(0)
{
}


MOSResource::~MOSResource()
{
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
		size_t decompressedSize = len;
		uint8* decompressedData = new uint8[decompressedSize];
		status_t status = ZLibDecompressor::DecompressBuffer(
							(uint8*)fData->Data() + std::ptrdiff_t(12),
							fData->Size() - 12, decompressedData, decompressedSize);
		if (status != 0) {
			delete[] decompressedData;
			return false;
		}

		ReplaceData(new MemoryStream(decompressedData, decompressedSize, true));
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

	//printf("width: %d, height: %d, columns: %d, rows: %d\n", fWidth, fHeight, fColumns, fRows);
	fTileOffsets = fPaletteOffset + kPaletteDataSize * fColumns * fRows;
	fPixelDataOffset = fTileOffsets + (uint32)fColumns * fRows * sizeof(uint32);

	return true;
}


Bitmap*
MOSResource::Image()
{
	GFX::rect tileRect( 0, 0, uint16(fBlockSize), uint16(fBlockSize));
	Bitmap* bitmap = new Bitmap(fWidth, fHeight, 16);
	for (uint16 y = 0; y < fRows; y++) {
		tileRect.y = y * fBlockSize;
		for (uint16 x = 0; x < fColumns; x++) {
			tileRect.x = x * fBlockSize;
			const uint32 tileNum = y * fColumns + x;
			Bitmap* tile = TileAt(tileNum);
			if (tile != NULL) {
				GraphicsEngine::BlitBitmap(tile, NULL, bitmap, &tileRect);
				tile->Release();
			} else {
				std::cerr << "MOSResource::Image(): tile number ";
				std::cerr << tileNum << " is NULL" << std::endl;
			}
		}
	}
	return bitmap;
}


Bitmap*
MOSResource::TileAt(uint32 index)
{
	fData->Seek(fPaletteOffset + index * kPaletteDataSize, SEEK_SET);

	uint16 xBlockSize = fBlockSize;
	uint16 yBlockSize = fBlockSize;

	// The last row and column tiles could be smaller
	uint16 yRow = index / fColumns;
	uint16 xRow = index - yRow * fColumns;
	if (xRow == fColumns - 1)
		xBlockSize = fWidth - (fColumns - 1) * fBlockSize;
	if (yRow == fRows - 1)
		yBlockSize = fHeight - (fRows - 1) * fBlockSize;

	Bitmap* tileBitmap = new Bitmap(xBlockSize, yBlockSize, 8);
	try {
		GFX::Palette palette;
		for (int32 i = 0; i < 256; i++) {
			palette.colors[i].b = fData->ReadByte();
			palette.colors[i].g = fData->ReadByte();
			palette.colors[i].r = fData->ReadByte();
			palette.colors[i].a = fData->ReadByte();
		}

		uint32 tileOffset;
		fData->ReadAt(fTileOffsets + index * sizeof(uint32), tileOffset);
		fData->Seek(fPixelDataOffset + tileOffset, SEEK_SET);

		for (int y = 0; y < yBlockSize; y++) {
			uint8* pixels = (uint8*)tileBitmap->Pixels() + y * tileBitmap->Pitch();
			for (int x = 0; x < xBlockSize; x++) {
				uint8 pixel = fData->ReadByte();
				pixels[x] = pixel;
			}
		}
		tileBitmap->SetPalette(palette);
	} catch (std::exception& e) {
		std::cerr << "MOSResource::TileAt()" << e.what() << std::endl;
		tileBitmap->Release();
		tileBitmap = NULL;
	} catch (...) {
		tileBitmap->Release();
		tileBitmap = NULL;
	}

	return tileBitmap;
}
