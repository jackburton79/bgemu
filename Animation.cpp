#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "Graphics.h"
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
	fBlackAsTransparent = animDesc->flags & ANIM_SHADED;
	fMirrored = animDesc->flags & ANIM_MIRRORED;
	printf("%s: %s\n", (const char*)animDesc->bam_name, fBlackAsTransparent ? "transparent": "black");
	//printf("palette: %s\n", (const char *)animDesc->palette);
	printf("transparency: %d\n", animDesc->transparency);
}


Animation::Animation(Actor *actor)
:
	fBAM(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0),
	fBlackAsTransparent(false),
	fMirrored(false)
{
	fBAM = gResManager->GetBAM(Actor::AnimationFor(*actor));
	fCycleNumber = actor->Orientation();
	if (fCycleNumber > 4)
		fCycleNumber = 4;

	fCenter = actor->Position();
	fCurrentFrame = 0;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);
	fHold = false;
	//printf("palette: %s\n", (const char *)animDesc->palette);
}


Animation::~Animation()
{
	gResManager->ReleaseResource(fBAM);
}


Frame
Animation::NextFrame()
{
	Frame frame = fBAM->FrameForCycle(fCycleNumber, fCurrentFrame);
	if (fMirrored) {
		frame.surface = Graphics::MirrorSDLSurface(frame.surface);
		frame.rect.x = frame.rect.x - frame.rect.w;
	}
	if (fBlackAsTransparent) {
		// TODO: How to do that ?
		//SDL_SetColorKey(frame.surface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 255);

	}
	if (!fHold) {
		fCurrentFrame++;
		if (fCurrentFrame >= fMaxFrame)
			fCurrentFrame = 0;
	}
	return frame;
}
