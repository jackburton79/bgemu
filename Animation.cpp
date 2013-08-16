#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "CreResource.h"
#include "BamResource.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "ResManager.h"

Animation::Animation(IE::animation *animDesc)
	:
	fBAM(NULL),
	fAnimation(animDesc),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0)
{
	memcpy(fName, animDesc->bam_name.name, sizeof(animDesc->bam_name.name));
	fName[8] = '\0';
	//strcpy(fName, animDesc->bam_name);

	fBAM = gResManager->GetBAM(fName);
	if (fBAM == NULL) {
		printf("NULL BAM!!!\n");
		throw -1;
	}
	fCycleNumber = animDesc->sequence;
	fCenter = animDesc->center;
	fCurrentFrame = animDesc->frame;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);
	fHold = animDesc->flags & IE::ANIM_HOLD;
	fBlackAsTransparent = animDesc->flags & IE::ANIM_SHADED;
	//if (fBlackAsTransparent)
		//fBAM->DumpFrames("/home/stefano/dumps");
	fMirrored = animDesc->flags & IE::ANIM_MIRRORED;
	printf("%s\n\t: SHADED: %s\n\t, MIRRORED: %s\n", (const char*)Name(),
			fBlackAsTransparent ? "YES": "NO",
			fMirrored ? "YES": "NO");
	//printf("palette: %s\n", (const char *)animDesc->palette);
	//printf("transparency: %d\n", animDesc->transparency);
}


Animation::Animation(const char* bamName,
		int sequence, IE::point position)
:
	fBAM(NULL),
	fAnimation(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0),
	fBlackAsTransparent(false),
	fMirrored(false)
{
	strcpy(fName, bamName);

	fBAM = gResManager->GetBAM(fName);
	if (fBAM == NULL)
		throw -1;

	fCycleNumber = sequence;
	if (fCycleNumber >= fBAM->CountCycles()) {
		std::cout << "CycleNumber exceeds cycles count: cyclenumber ";
		std::cout << fCycleNumber << ", cycles: " << int(fBAM->CountCycles());
		std::cout << std::endl;
		throw -1;
	}
	fCenter = position;
	fCurrentFrame = 0;
	fMaxFrame = fBAM->CountFrames(fCycleNumber);

	fHold = false;
}


Animation::~Animation()
{
	gResManager->ReleaseResource(fBAM);
}


bool
Animation::IsShown() const
{
	return fAnimation != NULL ? fAnimation->flags & IE::ANIM_SHOWN : true;
}


void
Animation::SetShown(const bool show)
{
	if (fAnimation != NULL)
		fAnimation->flags |= IE::ANIM_SHOWN;
}


void
Animation::SetMirrored(const bool mirror)
{
	fMirrored = mirror;
}


const char*
Animation::Name() const
{
	return fName;
}


const ::Bitmap*
Animation::Bitmap()
{
	const ::Bitmap* frame = fBAM->FrameForCycle(fCycleNumber, fCurrentFrame);
	if (fMirrored) {
		// TODO: We are mirroring on the fly. Maybe it's smarter
		// to cache the mirrored bitmaps.
		frame = frame->GetMirrored();

		std::cout << "Mirrored BITMAP" << std::endl;

	}
	if (fBlackAsTransparent) {
		// TODO: How to do that ?
		//frame.bitmap = Graphics::ApplyMask(frame.bitmap, NULL, 0, 0);

	}
	return frame;
}


void
Animation::Next()
{
	if (!fHold) {
		fCurrentFrame++;
		if (fCurrentFrame >= fMaxFrame)
			fCurrentFrame = 0;
	}
}


const ::Bitmap*
Animation::NextBitmap()
{
	Next();
	return Bitmap();
}


IE::point
Animation::Position() const
{
	return fCenter;
}
