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
	fMaxFrame(0),
	fType(0)
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


Animation::Animation(CREResource* cre, int action,
		int sequence, IE::point position)
:
	fBAM(NULL),
	fAnimation(NULL),
	fCycleNumber(0),
	fCurrentFrame(0),
	fMaxFrame(0),
	fType(0),
	fBlackAsTransparent(false),
	fMirrored(false)
{
	res_ref name = _SelectAnimation(cre, action, sequence);

	strncpy(fName, name.CString(), sizeof(name.name));

	fBAM = gResManager->GetBAM(fName);
	if (fBAM == NULL)
		throw -1;

	fCycleNumber = sequence;
	if (fCycleNumber >= fBAM->CountCycles())
		fCycleNumber = fBAM->CountCycles() - 1;
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


const char*
Animation::Name() const
{
	return fName;
}


::Frame
Animation::Frame()
{
	::Frame frame = fBAM->FrameForCycle(fCycleNumber, fCurrentFrame);
	if (fMirrored) {
		// TODO: Since BAM can cache the frames,
		// what we are doing here is wrong, we could end up
		// mirroring the frame once again.
		// We can't move the frame caching to Animation,
		// since we would end up caching the same BAM frames
		// for every animation.
		// Find a better way
		//GraphicsEngine::MirrorBitmap(frame.bitmap, GraphicsEngine::MIRROR_HORIZONTALLY);
		//frame.rect.x = frame.rect.x - frame.rect.w;
	}
	if (fBlackAsTransparent) {
		// TODO: How to do that ?
		Graphics::ApplyMask(frame.bitmap, NULL, 0, 0);

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


::Frame
Animation::NextFrame()
{
	Next();
	return Frame();
}


IE::point
Animation::Position() const
{
	return fCenter;
}


res_ref
Animation::_SelectAnimation(CREResource* cre, int action, int orientation)
{
	const uint16 animationID = cre->AnimationID();
	//printf("%s\n", 	AnimateIDS()->ValueFor(animationID));

	res_ref nameRef;
	uint8 high = (animationID & 0xFF00) >> 8;
	//uint8 low = (animationID & 0x00FF);
	// TODO: Seems like animation type could be told by
	// a mask here: monsters, characters, "objects", etc.

	//fType = AnimationType(animationID);
	//if (fType != ANIMATION_CHARACTER) {
	std::string baseName = AniSndIDS()->ValueFor(animationID);
	if (baseName.empty()) {
		printf("unknown code 0x%x (%s)\n", animationID,
					AnimateIDS()->ValueFor(animationID));
		throw -1;
	}

	if ((high >= 0xb2 && high <= 0xb4)
			|| (high >= 0xc6 && high <= 0xc9)
			|| (baseName[0] == 'N'
					&& (baseName[1] == 'N'
						|| baseName[1] == 'S'))) {
		// TODO: Low/High ?
		if (true)
			baseName.append("L");
		else
			baseName.append("H");
	}
	if (baseName[0] == 'M') {
		switch (action) {
			//case ACT_WALKING: baseName.append("G11"); break;
			case ACT_STANDING:
			default: baseName.append("G1"); break;
		}
		if (orientation >= IE::ORIENTATION_NE
			&& orientation <= IE::ORIENTATION_SE) {
			fMirrored = true;
		}
	} else if (baseName[0] == 'C') {
		// TODO: Can not work like this. Find out how to implement
		if (baseName[3] != 'F' && baseName[3] != 'C')
			baseName[4] = '2';
		else
			baseName[4] = '4'; //ArmorToLetter();
		baseName[5] = 'G'; // Action
		if (baseName[5] == 'A' || baseName[5] == 'G')
			baseName[6] = '1'; // Detail
		else if (baseName[5] == 'W')
			baseName[6] = '2'; // Detail
		else
			baseName[6] = '\0';

		baseName[7] = '\0';

	} else if (baseName[0] == 'S') {
		baseName.append("C");
	} else {
		baseName.append("G1");
		if (baseName[0] == 'N') {
			if (orientation >= IE::ORIENTATION_NE
					&& orientation <= IE::ORIENTATION_SE)
				baseName.append("E");
		}
	}

	strcpy(nameRef.name, baseName.c_str());

	//printf("nameRef: %s\n", (const char *)nameRef);
	return nameRef;
}
