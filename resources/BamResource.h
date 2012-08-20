#ifndef __BAMRESOURCE_H
#define __BAMRESOURCE_H

#include "Frame.h"
#include "Resource.h"

struct cycle {
	uint16 numFrames;
	uint16 index;
};

struct Palette;
class BAMResource : public Resource {
public:
	BAMResource(const res_ref& name);

	~BAMResource();
	
	bool Load(Archive *archive, uint32 key);

	Frame FrameForCycle(uint8 cycleIndex, uint16 frameIndex);
	
	uint16 CountFrames() const;
	uint16 CountFrames(uint8 cycleIndex);
	uint8 CountCycles() const;
	
	void DumpFrames(const char *path);

private:
	void _Load();
	uint8 _FindTransparentIndex();

	Frame _FrameAt(uint16 index);

	Palette *fPalette;
	uint32 fFramesOffset;
	uint32 fCyclesOffset;
	uint32 fFrameLookupOffset;
	
	uint16 fNumFrames;
	uint8 fNumCycles;
	uint8 fCompressedIndex;
	uint8 fTransparentIndex;
};


#endif
