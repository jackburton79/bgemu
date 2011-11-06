#ifndef __BAMRESOURCE_H
#define __BAMRESOURCE_H

#include "Resource.h"
#include "SDL.h"
#include "TileCell.h"

struct cycle {
	uint16 numFrames;
	uint16 index;
};

class BAMResource : public Resource {
public:
	BAMResource(uint8 *data, uint32 size, uint32 key);
	BAMResource(const res_ref& name);

	~BAMResource();
	
	bool Load(TArchive *archive, uint32 key);

	Frame FrameForCycle(int frameIndex, ::cycle *cycle);
	
	cycle *CycleAt(int index);
	
	uint16 CountFrames() const;
	uint8 CountCycles() const;
	
	void DumpFrames(const char *path);

private:
	void _Load();
	uint8 _FindTransparentIndex();

	Frame _FrameAt(uint16 index);

	SDL_Color *fPalette;
	uint32 fFramesOffset;
	uint32 fCyclesOffset;
	uint32 fFrameLookupOffset;
	
	uint16 fNumFrames;
	uint8 fNumCycles;
	uint8 fCompressedIndex;
	uint8 fTransparentIndex;
};


#endif
