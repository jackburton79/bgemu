#ifndef __VVCRESOURCE_H
#define __VVCRESOURCE_H

#include "Resource.h"

#include <map>

namespace GFX {
	struct Palette;
}

enum effect_display_flags {
	EFF_DISPLAY_TRANSPARENT = 0,
	EFF_DISPLAY_TRANSLUCENT = 1,
	EFF_DISPLAY_TRANSLUCENT_SHADOW = 1 << 1,
	EFF_DISPLAY_BLENDED = 1 << 2,
	EFF_DISPLAY_MIRROR_X = 1 << 3,
	EFF_DISPLAY_MIRROR_Y = 1 << 4,
};

class Bitmap;
class VVCResource : public Resource {
public:
	VVCResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	virtual void Dump();

	res_ref BAMName() const;
	uint16 DisplayFlags();
	uint16 ColourFlags();
	
	uint32 CountFrames() const;
	uint32 IntroSequenceIndex() const;
	uint32 MiddleSequenceIndex() const;
	uint32 EndingSequenceIndex() const;

	uint32 XPosition();
	uint32 ZOffset();
	uint32 YPosition();
	
private:
	virtual ~VVCResource();

};


#endif
