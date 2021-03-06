#include "Animation.h"

#include "Actor.h"
#include "Core.h"
#include "CreResource.h"
#include "BamResource.h"
#include "Graphics.h"
#include "GraphicsEngine.h"
#include "IDSResource.h"
#include "ResManager.h"
#include "Utils.h"

#include <sstream>

Animation::Animation(IE::animation *animDesc)
	:
	fAnimation(animDesc),
	fCurrentFrame(0),
	fStartFrame(0),
	fMaxFrame(0)
{
	fName.append(animDesc->bam_name.name, sizeof(animDesc->bam_name.name));
	BAMResource* bam = gResManager->GetBAM(fName.c_str());
	if (bam == NULL) {
		throw std::runtime_error("Animation: Missing BAM");
	}
	
	//animDesc->Print();
	fCenter = animDesc->center;
	fMaxFrame = bam->CountFrames(animDesc->sequence);
	if (animDesc->flags & IE::ANIM_STOP_AT_FRAME)
		fMaxFrame = animDesc->frame;
	if (animDesc->flags & IE::ANIM_RANDOM_START_FRAME)
		fStartFrame = Core::RandomNumber(0, fMaxFrame);
	fCurrentFrame = fStartFrame;
	
	fBlackAsTransparent = animDesc->flags & IE::ANIM_SHADED;
	fMirrored = animDesc->flags & IE::ANIM_MIRRORED;
	
	_LoadBitmaps(bam, animDesc->sequence, NULL);

	gResManager->ReleaseResource(bam);
}


Animation::Animation(const char* bamName,
		int sequence, bool mirror, IE::point position,
		CREColors* colors)
	:
	fAnimation(NULL),
	fCurrentFrame(0),
	fStartFrame(0),
	fMaxFrame(0),
	fBlackAsTransparent(false),
	fMirrored(mirror),
	fName(bamName)
{
	BAMResource* bam = gResManager->GetBAM(fName.c_str());
	if (bam == NULL)
		throw std::runtime_error("Animation: Missing BAM");

	if (sequence >= bam->CountCycles()) {
		std::cout << "CycleNumber exceeds cycles count: cyclenumber ";
		std::cout << sequence << ", cycles: " << int(bam->CountCycles());
		std::cout << std::endl;
		throw std::runtime_error("Animation: Wrong cycle!");
	}
	fCenter = position;
	fCurrentFrame = 0;
	fMaxFrame = bam->CountFrames(sequence);

	_LoadBitmaps(bam, sequence, colors);

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
	if (fAnimation != NULL) {
		uint32 flags = fAnimation->flags;
		if (show)
			flags |= IE::ANIM_SHOWN;
		else
			flags &= ~IE::ANIM_SHOWN;
		fAnimation->flags = flags;
	}
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
	fCurrentFrame++;
	if (fCurrentFrame >= fMaxFrame)
		fCurrentFrame = fStartFrame;
}


const ::Bitmap*
Animation::NextBitmap()
{
	Next();
	return Animation::Bitmap();
}


bool
Animation::IsLastFrame() const
{
	return fCurrentFrame == fMaxFrame - 1;
}


IE::point
Animation::Position() const
{
	return fCenter;
}


void
Animation::_LoadBitmaps(BAMResource* bam, int16 sequence, CREColors* patchColors)
{
	for (int16 i = 0; i < fMaxFrame; i++) {
		::Bitmap* bitmap = bam->FrameForCycle(sequence, i);
		
		if (patchColors != NULL)
			_ApplyColorMODs(bitmap, patchColors);

		if (fBlackAsTransparent)
			Graphics::ApplyShade(bitmap);
		
		if (fMirrored)
			bitmap->Mirror();

		fBitmaps.push_back(bitmap);
	}
}


void
Animation::_ApplyColorMODs(::Bitmap *bitmap, CREColors *patchColors)
{
	GFX::Palette palette;
	bitmap->GetPalette(palette);

	int i;
	//for (i = 0; i < 4; ++i)
		//col[i] = src->col[i];

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x04 + i, patchColors->metal);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x10 + i, patchColors->minor);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x1c + i, patchColors->major);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x28 + i, patchColors->skin);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x34 + i, patchColors->leather);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x40 + i, patchColors->armor);

	for (i = 0; i < 12; ++i)
		palette.ModColor(0x4c + i, patchColors->hair);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x58 + i, patchColors->minor);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x60 + i, patchColors->major);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x68 + i, patchColors->minor);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x70 + i, patchColors->metal);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x78 + i, patchColors->leather);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x80 + i, patchColors->leather);

	for (i = 0; i < 8; ++i)
		palette.ModColor(0x88 + i, patchColors->minor);

	for (i = 0; i < 24; ++i)
		palette.ModColor(0x90 + i, patchColors->leather);

	//for (i = 0; i < 8; ++i)
	//	palette.colors[[0xA8 + i] = src->col[0xA8+i];

	for (i = 0; i < 8; ++i)
		palette.ModColor(0xB0 + i, patchColors->skin);

	for (i = 0; i < 72; ++i)
		palette.ModColor(0xB8 + i, patchColors->leather);

	bitmap->SetPalette(palette);
}
