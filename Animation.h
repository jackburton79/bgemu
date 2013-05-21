#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "Frame.h"
#include "IETypes.h"

#include "Referenceable.h"

enum animation_action {
	ACT_WALKING = 10,
	ACT_STANDING,
	ACT_ATTACKING
};


class Actor;
class BAMResource;
class CREResource;
class Animation : public Referenceable {
public:
	Animation(IE::animation *animDesc);
	Animation(const char* bamName, int sequence, IE::point position);
	~Animation();

	const char* Name() const;

	bool IsShown() const;
	void SetShown(const bool show);

	void SetMirrored(const bool mirror);

	::Frame Frame();
	void Next();
	::Frame NextFrame();

	IE::point Position() const;

private:
	BAMResource *fBAM;
	IE::animation *fAnimation;
	IE::point fCenter;
	int16 fCycleNumber;
	int16 fCurrentFrame;
	uint16 fMaxFrame;
	bool fHold;
	bool fBlackAsTransparent;
	bool fMirrored;
	char fName[16];
};

#endif // __ANIMATION_H
