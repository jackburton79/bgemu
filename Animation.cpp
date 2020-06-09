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
		throw "NULL BAM!!!";
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
		
		if (patchColors != NULL) {
			GFX::Palette palette;
			bitmap->GetPalette(palette);
			GFX::Color hair = palette.colors[patchColors->hair];
			GFX::Color skin = palette.colors[patchColors->skin];
			//GFX::Color major = palette.colors[patchColors->major];
			//GFX::Color minor = palette.colors[patchColors->minor];
			//GFX::Color metal = palette.colors[patchColors->metal];
			//GFX::Color leather = palette.colors[patchColors->leather];
			//GFX::Color armor = palette.colors[patchColors->armor];
			bitmap->SetColors(skin, 45, 5);
			/*
			//bitmap->SetColors(major, 0, 10);
			bitmap->SetColors(minor, 10, 10);
			
			bitmap->SetColors(leather, 20, 10);*/
			//bitmap->SetColors(armor, 60, 10);
			bitmap->SetColors(hair, 80, 10);
			// 00-10 = shadow
			// 30-39 = vest1
			// 40-49 = skin
			// 50-59 = vest2
			// 60-69 = shoulders

		}

		if (fBlackAsTransparent)
			Graphics::ApplyShade(bitmap);
		
		if (fMirrored)
			bitmap->Mirror();

		fBitmaps.push_back(bitmap);
	}
}
