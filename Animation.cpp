#include "Actor.h"
#include "Animation.h"
#include "BamResource.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "ResManager.h"

Animation::Animation(IE::animation *animDesc)
	:
	fBAM(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0)
{
	fBAM = gResManager->GetBAM(animDesc->bam_name);
	if (fBAM == NULL)
		throw -1;
	fCycleNumber = animDesc->sequence;
	fCenter = animDesc->center;
	fCurrentFrame = animDesc->frame;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);
	fHold = animDesc->flags & IE::ANIM_HOLD;
	fBlackAsTransparent = animDesc->flags & IE::ANIM_SHADED;
	fMirrored = animDesc->flags & IE::ANIM_MIRRORED;
	//printf("%s: %s\n", (const char*)animDesc->bam_name, fBlackAsTransparent ? "transparent": "black");
	//printf("palette: %s\n", (const char *)animDesc->palette);
	//printf("transparency: %d\n", animDesc->transparency);
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
	if (fBAM == NULL)
		throw -1;

	fCycleNumber = actor->Orientation() % 4;
	if (fCycleNumber > fBAM->CountCycles())
		fCycleNumber = fBAM->CountCycles() - 1;
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
	if (fBAM == NULL)
		printf("BAM is NULL!!!!\n");

	Frame frame = fBAM->FrameForCycle(fCycleNumber, fCurrentFrame);
	if (fMirrored) {
		GraphicsEngine::MirrorBitmap(frame.bitmap, GraphicsEngine::MIRROR_HORIZONTALLY);
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
