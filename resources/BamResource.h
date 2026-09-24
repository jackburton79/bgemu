#ifndef __BAMRESOURCE_H
#define __BAMRESOURCE_H

#include "Resource.h"


namespace GFX {
	class Palette;
}

class Bitmap;
class BAMResource : public Resource {
public:
	BAMResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	virtual void Dump();

	Bitmap* FrameForCycle(uint8 cycleIndex, uint16 frameIndex);
	// The first frame of the cycle that isn't the same picture as its first one
	// (NULL if there is none): a BG1 paperdoll BAM holds the upper and the
	// lower half of the doll as the two pictures of its cycle 0.
	Bitmap* SecondPictureForCycle(uint8 cycleIndex);

	uint16 CountFrames() const;
	uint16 CountFrames(uint8 cycleIndex) const;
	uint8 CountCycles() const;

	uint8 TransparentIndex() const;

	void PrintFrames(uint8 cycleIndex) const;
	void DumpFrames(const char *path);

	static Resource* Create(const res_ref& name);

private:
	virtual ~BAMResource();
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
};


#endif
