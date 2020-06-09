#include "VVCResource.h"

#include "VVCResource.h"
#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "Path.h"
#include "IETypes.h"
#include "Utils.h"
#include "ZLibDecompressor.h"

#include <assert.h>
#include <iostream>
#include <limits.h>

#define VVC_SIGNATURE "VVC "
#define VVC_VERSION_1 "V1.0"


/* static */
Resource*
VVCResource::Create(const res_ref& name)
{
	return new VVCResource(name);
}


VVCResource::VVCResource(const res_ref& name)
	:
	Resource(name, RES_VVC)
{
}


VVCResource::~VVCResource()
{
}


bool
VVCResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	try {
		_Load();
	} catch (...) {
		return false;
	}
	return true;
}


void
VVCResource::Dump()
{
	std::cout << Name() << std::endl;
	std::cout << std::endl;
}


void
VVCResource::_Load()
{
	if (CheckSignature(VVC_SIGNATURE)) {
		if (!CheckVersion(VVC_VERSION_1))
			throw -1;

/*		uint32 len;
		fData->ReadAt(8, len);
		uint8 *decompressedData = new uint8[len];
		size_t decompressedSize = len;
		status_t status = ZLibDecompressor::DecompressBuffer(
							(uint8*)(fData->Data()) + std::ptrdiff_t(12),
							fData->Size() - 12, decompressedData, decompressedSize);

		if (status != 0) {
			delete[] decompressedData;
			throw -1;
		}

		ReplaceData(new MemoryStream(decompressedData, decompressedSize, true));
*/
	}

	// Fall through, since in the previous code block we
	// extracted a BAM file
/*
	if (CheckSignature(BAM_SIGNATURE)) {
		if (!CheckVersion(BAM_VERSION_1))
			throw -1;

		fData->ReadAt(8, fNumFrames);
		//std::cout << fNumFrames << " frames found." << std::endl;
		fData->ReadAt(10, fNumCycles);
		//std::cout << (int)fNumCycles << " cycles found." << std::endl;
		fData->ReadAt(11, fCompressedIndex);
		//std::cout << "Compressed index: " << (int)fCompressedIndex << std::endl;
		fData->ReadAt(12, fFramesOffset);
		fCyclesOffset = fFramesOffset + fNumFrames * sizeof(BamFrameEntry);

		uint32 paletteOffset;
		fData->ReadAt(16, paletteOffset);
		fData->ReadAt(20, fFrameLookupOffset);

		fData->Seek(paletteOffset, SEEK_SET);
		fPalette = new GFX::Palette;
		for (int32 i = 0; i < 256; i++) {
			fPalette->colors[i].b = fData->ReadByte();
			fPalette->colors[i].g = fData->ReadByte();
			fPalette->colors[i].r = fData->ReadByte();
			fPalette->colors[i].a = fData->ReadByte();
		}
	} else
		throw -1;

	fTransparentIndex = _FindTransparentIndex();
	*/
}

