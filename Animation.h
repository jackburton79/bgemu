#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "IETypes.h"

#include "Referenceable.h"

#include <vector>

enum animation_action {
	ACT_WALKING = 0,
	ACT_STANDING = 1,
	ACT_ATTACKING = 2
};


class Actor;
class BAMResource;
class Bitmap;
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

	const ::Bitmap* Bitmap();
	void Next();
	const ::Bitmap* NextBitmap();

	IE::point Position() const;

private:
	void _LoadBitmaps();

	BAMResource *fBAM;
	IE::animation *fAnimation;
	std::vector< ::Bitmap*> fBitmaps;

	IE::point fCenter;
	int16 fCycleNumber;
	int16 fCurrentFrame;
	uint16 fMaxFrame;
	bool fHold;
	bool fBlackAsTransparent;
	bool fMirrored;
	std::string fName;
};

#endif // __ANIMATION_H
