#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "Frame.h"
#include "IETypes.h"


class BAMResource;
class Animation {
public:
	Animation(animation *animDesc);
	Animation(Actor *actor);
	~Animation();

	Frame NextFrame();

//private:
	BAMResource *fBAM;
	point fCenter;
	int16 fCycleNumber;
	int16 fCurrentFrame;
	uint16 fMaxFrame;
	bool fHold;
	bool fBlackAsTransparent;
	bool fMirrored;
};

#endif // __ANIMATION_H
