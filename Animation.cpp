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
	fAnimation(animDesc),
	fCurrentFrame(0),
	fMaxFrame(0)
{
	fName.append(animDesc->bam_name.name, sizeof(animDesc->bam_name.name));
	BAMResource* bam = gResManager->GetBAM(fName.c_str());
	if (bam == NULL) {
		throw "NULL BAM!!!";
	}
	
	fCenter = animDesc->center;
	fCurrentFrame = animDesc->frame;
	fMaxFrame = bam->CountFrames(animDesc->sequence);
	fHold = animDesc->flags & IE::ANIM_HOLD;
	fBlackAsTransparent = animDesc->flags & IE::ANIM_SHADED;
	//if (fBlackAsTransparent)
		//fBAM->DumpFrames("/home/stefano/dumps");
	fMirrored = animDesc->flags & IE::ANIM_MIRRORED;
	printf("%s\n\t: SHADED: %s\n\t: MIRRORED: %s\n", (const char*)Name(),
			fBlackAsTransparent ? "YES": "NO",
			fMirrored ? "YES": "NO");
	printf("\t: MAX FRAME: %d\n", fMaxFrame);
	printf("\t: HOLD: %s\n", fHold ? "YES" : "NO");
	//printf("palette: %s\n", (const char *)animDesc->palette);
	//printf("transparency: %d\n", animDesc->transparency);

	_LoadBitmaps(bam, animDesc->sequence);

	gResManager->ReleaseResource(bam);
}


Animation::Animation(const char* bamName,
		int sequence, bool mirror, IE::point position)
	:
	fAnimation(NULL),
	fCurrentFrame(0),
	fMaxFrame(0),
	fBlackAsTransparent(false),
	fMirrored(mirror)
{
	fName = bamName;
	BAMResource* bam = gResManager->GetBAM(fName.c_str());
	if (bam == NULL)
		throw "missing BAM";

	if (sequence >= bam->CountCycles()) {
		std::cout << "CycleNumber exceeds cycles count: cyclenumber ";
		std::cout << sequence << ", cycles: " << int(bam->CountCycles());
		std::cout << std::endl;
		throw "Wrong cycle";
	}
	fCenter = position;
	fCurrentFrame = 0;
	fMaxFrame = bam->CountFrames(sequence);

	fHold = false;

	_LoadBitmaps(bam, sequence);

	gResManager->ReleaseResource(bam);
}


Animation::~Animation()
{
	for (std::vector< ::Bitmap*>::iterator i = fBitmaps.begin();
		i != fBitmaps.end(); i++) {
		(*i)->Release();
	}
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


const char*
Animation::Name() const
{
	return fName.c_str();
}


const ::Bitmap*
Animation::Bitmap()
{
	if ((size_t)fCurrentFrame >= fBitmaps.size())
		return NULL;

	const ::Bitmap* frame = fBitmaps.at(fCurrentFrame);
	
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
	return Animation::Bitmap();
}


IE::point
Animation::Position() const
{
	return fCenter;
}


void
Animation::_LoadBitmaps(BAMResource* bam, int16 sequence)
{
	for (int16 i = 0; i < fMaxFrame; i++) {
		::Bitmap* bitmap = bam->FrameForCycle(sequence, i);
		
		// TODO: Patch colors here.
		
		if (fBlackAsTransparent)
			Graphics::ApplyShade(bitmap);
		
		// TODO: Looks like we are leaking the original bitmap here
		if (fMirrored)
			bitmap = bitmap->GetMirrored();

		fBitmaps.push_back(bitmap);
	}
}
