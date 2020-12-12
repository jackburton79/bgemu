#include "BamResource.h"

#include "Bitmap.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "Log.h"
#include "MemoryStream.h"
#include "Path.h"
#include "IETypes.h"
#include "Utils.h"
#include "ZLibDecompressor.h"

#include <assert.h>
#include <iostream>
#include <limits.h>

#define BAM_SIGNATURE "BAM "
#define BAMC_SIGNATURE "BAMC"
#define BAM_VERSION_1 "V1  "


struct cycle {
	uint16 numFrames;
	uint16 index;
};

struct BamFrameEntry {
	uint16 width;
	uint16 height;
	uint16 xpos;
	uint16 ypos;
	uint32 data;
	void Print() const;
};


void
BamFrameEntry::Print() const
{
	std::cout << "width: " << width << std::endl;
	std::cout << "height: " << height << std::endl;
	std::cout << "xpos: " << xpos << std::endl;
	std::cout << "ypos: " << ypos << std::endl;
}


static inline bool
is_frame_compressed(uint32 x)
{
	return (x >> 31) == 0;
}


static inline
uint32 data_offset(uint32 x)
{
	return x & 0x7FFFFFFF;
}


/* static */
Resource*
BAMResource::Create(const res_ref& name)
{
	return new BAMResource(name);
}


BAMResource::BAMResource(const res_ref& name)
	:
	Resource(name, RES_BAM),
	fPalette(NULL),
	fFramesOffset(0),
	fCyclesOffset(0),
	fFrameLookupOffset(0),
	fNumFrames(0),
	fNumCycles(0),
	fCompressedIndex(0),
	fTransparentIndex(0)
{
}


BAMResource::~BAMResource()
{
	delete fPalette;
}


bool
BAMResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	try {
		_Load();
	} catch (std::exception& e) {
		std::cerr << RED(e.what()) << std::endl;
		return false;
	}
	return true;
}


void
BAMResource::Dump()
{
	std::cout << "FrameOffset: " << std::dec << fFramesOffset << std::endl;
	std::cout << "CyclesOffset: " << fCyclesOffset << std::endl;
	std::cout << "FrameLookupOffset: " << fFrameLookupOffset << std::endl;

	std::cout << "NumFrames: " << fNumFrames << std::endl;
	std::cout << "NumCycles: " << fNumCycles << std::endl;

	for (int cycle = 0; cycle < fNumCycles; cycle++)
		PrintFrames(cycle);

	int32 oldPos = fData->Position();
	//size_t outputted = 0;
	for(int f = 0; f < fNumFrames; f++) {
		fData->Seek(fFramesOffset + f * sizeof(BamFrameEntry), SEEK_SET);
		BamFrameEntry entry;
		fData->Read(entry);
		if (entry.width != 1 && entry.height != 1) {
			std::cout << "BamFrameEntry " << f << std::endl;
			entry.Print();
			fData->Seek(data_offset(entry.data), SEEK_SET);
			/*try {
			uint8 byte = fData->ReadByte();
			std::cout << std::hex << (int)byte << " ";
			} catch (...) {
				break;
			}
			if (outputted++ >= sizeof(BamFrameEntry)) {
				outputted = 0;
				std::cout << std::endl << "**** offset: " << fData->Position();
				std::cout << " ****" << std::endl ;
			}*/
			std::cout << "****" << std::endl ;
		}
	}

	fData->Seek(oldPos, SEEK_SET);

	std::cout << std::endl;
}


uint8
BAMResource::_FindTransparentIndex()
{
	for (uint16 i = 0; i < 256; i++) {
		const GFX::Color *color = &fPalette->colors[i];
		if (color->r == 0 && color->g == 255 && color->b == 0) {
			return i;
		}
	}

	return 0;
}


void
BAMResource::PrintFrames(uint8 cycleIndex) const
{
	::cycle newCycle;
	fData->ReadAt(fCyclesOffset + (cycleIndex * sizeof(newCycle)), newCycle);
	std::cout << "Cycle " << (int)cycleIndex << ": ";
	std::cout << newCycle.numFrames << " frames:" << std::endl;
	for (uint16 numFrame = 0; numFrame < newCycle.numFrames; numFrame++) {
		uint16 index;
		fData->ReadAt(fFrameLookupOffset
				+ (newCycle.index + numFrame) * sizeof(int16), index);
		std::cout << numFrame << ": " << index << ", ";
	}
	std::cout << std::endl;
}


void
BAMResource::DumpFrames(const char *filePath)
{
	printf("DumpFrames: cycles: %d\n", fNumCycles);
	for (uint8 cycle = 0; cycle < fNumCycles; cycle++) {
		printf("cycle %d\n", cycle);
		int numFrames = CountFrames(cycle);
		printf("\tframes: %d\n", numFrames);
		for (int numFrame = 0; numFrame < numFrames; numFrame++) {
			try {
				const Bitmap* frame = FrameForCycle(cycle, numFrame);
				std::cout << "frame retrieved" << std::endl;
				Path path(filePath);
				char fileName[PATH_MAX];
				snprintf(fileName, PATH_MAX, "%s_CYCLE%d_FRAME%d.bmp",
						fName.CString(), cycle, numFrame);
				path.Append(fileName);
				printf("save to %s\n", path.String());
				frame->Save(path.String());
			} catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
				continue;
			}
		}
	}
}


Bitmap*
BAMResource::_FrameAt(uint16 index)
{
	BamFrameEntry entry;
	fData->ReadAt(fFramesOffset + index * sizeof(BamFrameEntry), entry);
	
	Bitmap* bitmap = NULL;
	uint8* bitmapData = NULL;
	bool ownsData = false;
	const uint32 offset = data_offset(entry.data);
	if (is_frame_compressed(entry.data)) {
		uint8* destData = new uint8[entry.width * entry.height];
		int decoded = Graphics::DecodeRLE((uint8*)(fData->Data()) + offset,
				entry.width * entry.height, destData,
				fCompressedIndex);
		if (decoded != entry.width * entry.height) {
			std::cout << "BAMResource::_FrameAt(): Failed to decode image!";
			delete[] destData;
			return NULL;
		}
		bitmapData = destData;
		ownsData = true;
	} else
		bitmapData = (uint8*)fData->Data() + std::ptrdiff_t(offset);

	bitmap = new DataBitmap(bitmapData, entry.width, entry.height, 8, ownsData);

	if (bitmap != NULL) {
		bitmap->SetPalette(*fPalette);
		bitmap->SetColorKey(fCompressedIndex, true);
		bitmap->SetPosition(entry.xpos - bitmap->Width() / 2,
			entry.ypos - bitmap->Height() / 2);
	} else if (ownsData)
		delete[] bitmapData;

	return bitmap;
}


Bitmap*
BAMResource::FrameForCycle(uint8 cycleIndex, uint16 frameIndex)
{
	//std::cout << "FrameForCycle: Cycle " << (int)cycleIndex << ", ";
	//std::cout << "Frame " << (int)frameIndex << std::endl;
	if (cycleIndex >= fNumCycles) {
		std::cerr << RED("BAMResource::FrameForCycle(): out of bounds!") << std::endl;
		return NULL;
	}

	::cycle newCycle;
	fData->ReadAt(fCyclesOffset + (cycleIndex * sizeof(cycle)), newCycle);

	uint16 index;
	fData->ReadAt(fFrameLookupOffset
			+ (newCycle.index + frameIndex) * sizeof(int16), index);

	return _FrameAt(index);
}


uint16
BAMResource::CountFrames() const
{
	return fNumFrames;
}


uint16
BAMResource::CountFrames(uint8 cycleIndex) const
{
	::cycle newCycle;
	fData->ReadAt(fCyclesOffset + (cycleIndex * sizeof(newCycle)), newCycle);
	return newCycle.numFrames;
}


uint8
BAMResource::CountCycles() const
{
	return fNumCycles;
}


uint8
BAMResource::TransparentIndex() const
{
	return fTransparentIndex;
}


void
BAMResource::_Load()
{
	if (CheckSignature(BAMC_SIGNATURE)) {
		if (!CheckVersion(BAM_VERSION_1))
			throw std::runtime_error("BAMResource::Load() wrong version!");

		uint32 len;
		fData->ReadAt(8, len);
		uint8 *decompressedData = new uint8[len];
		size_t decompressedSize = len;
		status_t status = ZLibDecompressor::DecompressBuffer(
							(uint8*)(fData->Data()) + std::ptrdiff_t(12),
							fData->Size() - 12, decompressedData, decompressedSize);

		if (status != 0) {
			delete[] decompressedData;
			throw std::runtime_error("BAMResource::Load() decompression failed!");
		}

		ReplaceData(new MemoryStream(decompressedData, decompressedSize, true));
	}

	// Fall through, since in the previous code block we
	// extracted a BAM file

	if (CheckSignature(BAM_SIGNATURE)) {
		if (!CheckVersion(BAM_VERSION_1))
			throw std::runtime_error("BAMResource::Load() wrong version!");

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
		throw std::runtime_error("BAMResource::Load() wrong signature!");

	fTransparentIndex = _FindTransparentIndex();
}
