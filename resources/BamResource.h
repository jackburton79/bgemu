#ifndef __BAMRESOURCE_H
#define __BAMRESOURCE_H

#include "Resource.h"

#include <map>

namespace GFX {
	struct Palette;
}

class Bitmap;
class BAMResource : public Resource {
public:
	BAMResource(const res_ref& name);
	~BAMResource();
	
	virtual bool Load(Archive *archive, uint32 key);

	virtual void Dump();

	const Bitmap* FrameForCycle(uint8 cycleIndex, uint16 frameIndex);
	
	uint16 CountFrames() const;
	uint16 CountFrames(uint8 cycleIndex) const;
	uint8 CountCycles() const;

	void PrintFrames(uint8 cycleIndex) const;
	void DumpFrames(const char *path);

private:
	void _Load();
	uint8 _FindTransparentIndex();

	Bitmap* _FrameAt(uint16 index);

	GFX::Palette *fPalette;
	uint32 fFramesOffset;
	uint32 fCyclesOffset;
	uint32 fFrameLookupOffset;
	
	uint16 fNumFrames;
	uint8 fNumCycles;
	uint8 fCompressedIndex;
	uint8 fTransparentIndex;

	std::map<std::pair<uint8, uint16>, Bitmap*> fFrames;
};


#endif
