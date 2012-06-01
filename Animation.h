#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "Frame.h"
#include "IETypes.h"


class BAMResource;
class Animation {
public:
	Animation(IE::animation *animDesc);
	Animation(Actor *actor);
	~Animation();

	Frame NextFrame();

//private:
	BAMResource *fBAM;
	IE::point fCenter;
	int16 fCycleNumber;
	int16 fCurrentFrame;
	uint16 fMaxFrame;
	//int fType;
	bool fHold;
	bool fBlackAsTransparent;
	bool fMirrored;
};

#endif // __ANIMATION_H
