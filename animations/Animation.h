#ifndef __ANIMATION_H
#define __ANIMATION_H

#include "IETypes.h"

#include "Referenceable.h"

#include <vector>

enum animation_action {
	ACT_WALKING = 0,
	ACT_STANDING = 1,
	ACT_ATTACKING = 2,
	ACT_DEAD = 3,
	ACT_CAST_SPELL = 4
};


class Actor;
class BAMResource;
class Bitmap;
class CREResource;
struct CREColors;
class Animation : public Referenceable {
public:
	Animation(IE::animation *animDesc);
	Animation(const char* bamName, int sequence, bool mirror,
		IE::point position, CREColors* colors = NULL);
	~Animation();

	const char* Name() const;

	bool IsShown() const;
	void SetShown(const bool show);

	const ::Bitmap* Bitmap();
	void Next();
	const ::Bitmap* NextBitmap();

	bool IsLastFrame() const;

	IE::point Position() const;

private:
	void _LoadBitmaps(BAMResource* bam, int16 sequence, CREColors* patchColors);
	void _ApplyColorMODs(::Bitmap* bitmap, CREColors* patchColors);

	IE::animation *fAnimation;
	std::vector< ::Bitmap*> fBitmaps;

	IE::point fCenter;
	int16 fCurrentFrame;
	uint16 fStartFrame;
	uint16 fMaxFrame;
	bool fHold;
	bool fBlackAsTransparent;
	bool fMirrored;
	std::string fName;
};

#endif // __ANIMATION_H
