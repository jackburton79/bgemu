#ifndef __ANIMATIONTESTER_H
#define __ANIMATIONTESTER_H

#include "IETypes.h"

#include <string>

class BAMResource;
class AnimationTester {
public:
	AnimationTester(std::string bamName);
	void SelectCycle(uint8 cycleNum);
	void UpdateAnimation();
	void Loop();
private:
	BAMResource* fResource;	
	uint8 fCycleNum;
	int32 fFrameNum;
	IE::point fPosition;
};

#endif
