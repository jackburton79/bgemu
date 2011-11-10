#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "Types.h"
#include "TileCell.h"


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
};

#endif // __ANIMATION_H
