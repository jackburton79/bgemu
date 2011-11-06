#include "BamResource.h"
#include "Graphics.h"
#include "MemoryStream.h"
#include "Path.h"
#include "Types.h"
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


BAMResource::BAMResource(uint8 *ptr, uint32 size, uint32 key)
	:
	Resource(ptr, size, key)
{
	fType = RES_BAM;
	_Load();
}


BAMResource::BAMResource(const res_ref& name)
	:
	Resource()
{
	fType = RES_BAM;
}


BAMResource::~BAMResource()
{
	delete[] fPalette;
}


bool
BAMResource::Load(TArchive *archive, uint32 key)
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
	for (uint8 i = 0; i < 256; i++) {
		const SDL_Color *color = &fPalette[i];
		if (color->r == 0 and color->g == 255 and color->b == 0) {
			printf("transparent index: %d\n", i);
			return i;
		}
	}

	return 0;
}


void
BAMResource::DumpFrames(const char *filePath)
{
#if 1
	for (int32 c = 0; c < fNumCycles; c++) {
		::cycle *cycle = CycleAt(c);
		for (int f = 0; f < fNumFrames; f++) {
			SDL_Surface *surface = FrameForCycle(f, cycle).surface;
			TPath path(filePath);
			char fileName[PATH_MAX];
			snprintf(fileName, PATH_MAX, "%sC%d_F%d.bmp",
					(const char *)fName, c, f);
			path.Append(fileName);
			printf("save to %s\n", path.Path());
			SDL_SaveBMP(surface, path.Path());
			SDL_FreeSurface(surface);
		}
		delete cycle;
	}
#else
	for (int f = 0; f < fNumFrames; f++) {
		SDL_Surface *surface = _FrameAt(f).surface;
		TPath path(filePath);
		char fileName[PATH_MAX];
		snprintf(fileName, PATH_MAX, "%s%d.bmp", (const char *)fName, f);
		path.Append(fileName);
		printf("save to %s\n", path.Path());
		SDL_SaveBMP(surface, path.Path());
		SDL_FreeSurface(surface);
	}
#endif
}


Frame
BAMResource::_FrameAt(uint16 index)
{
	BamFrameEntry entry;
	ReadAt(fFramesOffset + index * sizeof(BamFrameEntry), entry);
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
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY|SDL_RLEACCEL,
			fTransparentIndex);

	SDL_Rect rect;
	rect.w = surface->w;
	rect.h = surface->h;
	rect.x = entry.xpos;
	rect.y = entry.ypos;
	
	Frame frame;
	frame.surface = surface;
	frame.rect = rect;
	
	return frame;
}


Frame
BAMResource::FrameForCycle(int frameIndex, ::cycle *cycle)
{
	uint16 index;
	ReadAt(fFrameLookupOffset + (cycle->index + frameIndex) * sizeof(int16), index);
	return _FrameAt(index);
}


cycle *
BAMResource::CycleAt(int index)
{
	cycle *newCycle = new cycle;
	ReadAt(fCyclesOffset + index * sizeof(cycle), *newCycle);
	return newCycle;
}


uint16
BAMResource::CountFrames() const
{
	return fNumFrames;
}


uint8
BAMResource::CountCycles() const
{
	return fNumCycles;
}


void
BAMResource::_Load()
{
	char signature[5];
	signature[4] = '\0';
	char version[5];
	version[4] = '\0';
	ReadAt(0, signature, 4);
	if (!strcmp(signature, BAMC_SIGNATURE)) {
		ReadAt(4, version, 4);
		if (strcmp(version, BAM_VERSION_1)) {
			printf("invalid version: %s\n", version);
			throw -1;
		}

		uint32 len;
		ReadAt(8, len);
		uint8 *decompressedData = new uint8[len];
		int status = uncompress((Bytef*)decompressedData,
			(uLongf*)&len, (const Bytef*)(fData->Data()) + 12, fData->Size() - 12);

		if (status != Z_OK)
			throw -1;

		ReplaceData(new TMemoryStream(decompressedData, len, true));
	}

	ReadAt(0, signature, 4);
	if (!strcmp(signature, BAM_SIGNATURE)) {
		ReadAt(4, version, 4);
		if (strcmp(version, BAM_VERSION_1)) {
			printf("invalid version: %s\n", version);
			throw -1;
		}
		ReadAt(8, fNumFrames);
		std::cout << fNumFrames << " frames found." << std::endl;
		ReadAt(10, fNumCycles);
		//std::cout << (int)fNumCycles << " cycles found." << std::endl;
		ReadAt(11, fCompressedIndex);
		//std::cout << "Compressed index: " << (int)fCompressedIndex << std::endl;
		ReadAt(12, fFramesOffset);
		fCyclesOffset = fFramesOffset + fNumFrames * sizeof(BamFrameEntry);

		uint32 paletteOffset;
		ReadAt(16, paletteOffset);
		ReadAt(20, fFrameLookupOffset);

		Seek(paletteOffset, SEEK_SET);
		fPalette = new SDL_Color[256];
		for (int32 i = 0; i < 256; i++) {
			fPalette[i].b = ReadByte();
			fPalette[i].g = ReadByte();
			fPalette[i].r = ReadByte();
			fPalette[i].unused = ReadByte();
		}
	} else {
		printf("invalid signature: %s\n", signature);
		throw -1;
	}

	fTransparentIndex = _FindTransparentIndex();
}
