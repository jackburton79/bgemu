#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "ResManager.h"

Animation::Animation(animation *animDesc)
	:
	fBAM(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0)
{
	fBAM = gResManager->GetBAM(animDesc->bam_name);
	fCycleNumber = animDesc->sequence;
	fCenter = animDesc->center;
	fCurrentFrame = animDesc->frame;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);
	fHold = animDesc->flags & ANIM_HOLD;
}


Animation::Animation(Actor *actor)
:
	fBAM(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0)
{
	fBAM = gResManager->GetBAM(Actor::AnimationFor(*actor));
	fCycleNumber = actor->Orientation();
	if (fCycleNumber > 4)
		fCycleNumber = 4;

	fCenter = actor->Position();
	fCurrentFrame = 0;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);
	fHold = false;
}


Animation::~Animation()
{
	gResManager->ReleaseResource(fBAM);
}


Frame
Animation::NextFrame()
{
	Frame frame = fBAM->FrameForCycle(fCycleNumber, fCurrentFrame);
	if (!fHold) {
		fCurrentFrame++;
		if (fCurrentFrame >= fMaxFrame)
			fCurrentFrame = 0;
	}
	return frame;
}
