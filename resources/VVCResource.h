#ifndef __VVCRESOURCE_H
#define __VVCRESOURCE_H

#include "Resource.h"

#include <map>

namespace GFX {
	struct Palette;
}

class Bitmap;
class VVCResource : public Resource {
public:
	VVCResource(const res_ref& name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	virtual void Dump();

	res_ref BAMName() const;
	uint32 CountFrames() const;
	uint32 IntroSequenceIndex() const;
	uint32 MiddleSequenceIndex() const;
	uint32 EndingSequenceIndex() const;

private:
	virtual ~VVCResource();

};


#endif
