#include "Effect.h"

#include "BamResource.h"
#include "Bitmap.h"
#include "ResManager.h"
#include "VVCResource.h"

Effect::Effect(const res_ref& name, IE::point where)
	:
	fVVC(NULL),
	fCurrentFrame(0),
	fNumFrames(0),
	fWhere(where)
{
	fVVC = gResManager->GetVVC(name);
	fNumFrames = fVVC->CountFrames();
	_LoadBitmaps();
}


Effect::~Effect()
{
	gResManager->ReleaseResource(fVVC);
}


uint32
Effect::CountFrames() const
{
	return fNumFrames;
}


const ::Bitmap*
Effect::NextBitmap()
{
	if (fCurrentFrame >= fNumFrames)
		return NULL;

	const ::Bitmap* bitmap = fBitmaps.at(fCurrentFrame);
	fCurrentFrame++;

	return bitmap;
}


IE::point
Effect::Position() const
{
	return fWhere;
}


bool
Effect::Finished() const
{
	return fCurrentFrame >= fNumFrames;
}


void
Effect::_LoadBitmaps()
{
	BAMResource* bam = gResManager->GetBAM(fVVC->BAMName());
	uint32 firstSequence = fVVC->IntroSequenceIndex();
	uint32 secondSequence = fVVC->MiddleSequenceIndex();
	uint32 thirdSequence = fVVC->EndingSequenceIndex();
	std::cout << "bamname: " << fVVC->BAMName() << std::endl;
	std::cout << "first sequence:" << firstSequence << std::endl;
	std::cout << "second sequence:" << secondSequence << std::endl;
	std::cout << "third sequence:" << thirdSequence << std::endl;

	for (int16 i = 0; i < bam->CountFrames(firstSequence); i++) {
		::Bitmap* bitmap = bam->FrameForCycle(firstSequence, i);
		if (fVVC->DisplayFlags() & EFF_DISPLAY_MIRROR_X)
			bitmap->Mirror();
		//GFX::Palette palette;
		//bitmap->GetPalette(palette);
		fBitmaps.push_back(bitmap);
	}/*
	for (int16 i = 0; i < bam->CountFrames(secondSequence); i++) {
		::Bitmap* bitmap = bam->FrameForCycle(secondSequence, i);
		//GFX::Palette palette;
		//bitmap->GetPalette(palette);
		fBitmaps.push_back(bitmap);
	}
	for (int16 i = 0; i < bam->CountFrames(thirdSequence); i++) {
		::Bitmap* bitmap = bam->FrameForCycle(thirdSequence, i);
		//GFX::Palette palette;
		//bitmap->GetPalette(palette);
		fBitmaps.push_back(bitmap);
	}*/
	gResManager->ReleaseResource(bam);
}
