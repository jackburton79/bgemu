#include "BamResource.h"
#include "Graphics.h"
#include "MemoryStream.h"
#include "Path.h"
#include "IETypes.h"
#include "Utils.h"

#include <assert.h>
#include <iostream>
#include <limits.h>

#include <zlib.h>

#include <SDL.h>

#define BAM_SIGNATURE "BAM "
#define BAMC_SIGNATURE "BAMC"
#define BAM_VERSION_1 "V1  "


struct BamFrameEntry {
	uint16 width;
	uint16 height;
	uint16 xpos;
	uint16 ypos;
	uint32 data;
};


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


BAMResource::BAMResource(const res_ref& name)
	:
	Resource(name, RES_BAM)
{
}


BAMResource::~BAMResource()
{
	delete[] fPalette;
}


bool
BAMResource::Load(Archive *archive, uint32 key)
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


uint8
BAMResource::_FindTransparentIndex()
{
	for (uint16 i = 0; i < 256; i++) {
		const SDL_Color *color = &fPalette[i];
		if (color->r == 0 and color->g == 255 and color->b == 0) {
			//printf("transparent index: %d\n", i);
			return i;
		}
	}

	return 0;
}


void
BAMResource::DumpFrames(const char *filePath)
{
	for (int32 c = 0; c < fNumCycles; c++) {
		for (int f = 0; f < fNumFrames; f++) {
			SDL_Surface *surface = FrameForCycle(f, c).surface;
			TPath path(filePath);
			char fileName[PATH_MAX];
			snprintf(fileName, PATH_MAX, "%sC%d_F%d.bmp",
					(const char *)fName, c, f);
			path.Append(fileName);
			printf("save to %s\n", path.Path());
			SDL_SaveBMP(surface, path.Path());
			SDL_FreeSurface(surface);
		}
	}
}


Frame
BAMResource::_FrameAt(uint16 index)
{
	BamFrameEntry entry;
	fData->ReadAt(fFramesOffset + index * sizeof(BamFrameEntry), entry);
	//std::cout << "frame " << (int)index << std::endl;
	//std::cout << "width: " << entry.width << ", height: " << entry.height << std::endl;
	//std::cout << entry.xpos << " " << entry.ypos << std::endl;
	//std::cout << "data offset: " << data_offset(entry.data) << std::endl;
	bool frameCompressed = is_frame_compressed(entry.data);
	//std::cout << "frame is " << (frameCompressed ? "compressed" : "not compressed") << std::endl;
	
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
								entry.width, entry.height,
								8, 0, 0, 0, 0);
	
	const uint32 offset = data_offset(entry.data);
	int decoded = 0;
	uint8 *destData = new uint8[entry.width * entry.height];
	if (frameCompressed) {
		decoded = Graphics::DecodeRLE((uint8 *)(fData->Data()) + offset,
				entry.width * entry.height, destData,
				fCompressedIndex);
	} else {
		decoded = Graphics::Decode((uint8 *)(fData->Data()) + offset,
				entry.width * entry.height, destData);
	}

	Graphics::DataToSDLSurface(destData, entry.width, entry.height, surface);

	delete[] destData;
	
	if (decoded != entry.width * entry.height)
		throw -1;
	
	SDL_SetColors(surface, fPalette, 0, 256);
	uint32 color = 0;
	/*if (fTransparentIndex != 0) {
		const SDL_Color *trans = &fPalette[fTransparentIndex];
		color  = SDL_MapRGB(surface->format, trans->r, trans->g, trans->b);
	}*/
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY|SDL_RLEACCEL, color);

	SDL_Rect rect;
	rect.w = surface->w;
	rect.h = surface->h;
	rect.x = entry.xpos - rect.w / 2;
	rect.y = entry.ypos - rect.h / 2;
	
	Frame frame;
	frame.surface = surface;
	frame.rect = rect;
	
	return frame;
}


Frame
BAMResource::FrameForCycle(uint8 cycleIndex, uint16 frameIndex)
{
	if (cycleIndex >= fNumCycles) {
		printf("BAMResource::FrameForCycle(): out of bounds!\n");
		throw cycleIndex;
	}

	::cycle newCycle;
	fData->ReadAt(fCyclesOffset + (cycleIndex * sizeof(newCycle)), newCycle);

	uint16 index;
	fData->ReadAt(fFrameLookupOffset + (newCycle.index + frameIndex) * sizeof(int16), index);
	return _FrameAt(index);
}


uint16
BAMResource::CountFrames() const
{
	return fNumFrames;
}


uint16
BAMResource::CountFrames(uint8 cycleIndex)
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


void
BAMResource::_Load()
{
	if (CheckSignature(BAMC_SIGNATURE)) {
		if (!CheckVersion(BAM_VERSION_1))
			throw -1;

		uint32 len;
		fData->ReadAt(8, len);
		uint8 *decompressedData = new uint8[len];
		int status = uncompress((Bytef*)decompressedData,
			(uLongf*)&len, (const Bytef*)(fData->Data()) + 12, fData->Size() - 12);

		if (status != Z_OK)
			throw -1;

		ReplaceData(new MemoryStream(decompressedData, len, true));
	}

	// Fall through, since in the previous code block we
	// extracted a BAM file

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
		fPalette = new SDL_Color[256];
		for (int32 i = 0; i < 256; i++) {
			fPalette[i].b = fData->ReadByte();
			fPalette[i].g = fData->ReadByte();
			fPalette[i].r = fData->ReadByte();
			fPalette[i].unused = fData->ReadByte();
			//printf("%d: %d %d %d\n", i, fPalette[i].r,
				//	fPalette[i].g, fPalette[i].b);
		}
	} else
		throw -1;

	fTransparentIndex = _FindTransparentIndex();
}
