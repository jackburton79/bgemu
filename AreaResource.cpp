#include "AreaResource.h"


#define AREA_SIGNATURE "AREA"
#define AREA_VERSION_1 "V1.0"


AREAResource::AREAResource(const res_ref& name)
	:
	Resource(),
	fWedName(NULL),
	fAnimationsOffset(0),
	fNumAnimations(0),
	fAnimations(NULL)
{
	fType = RES_AREA;
}


AREAResource::~AREAResource()
{
	delete[] fAnimations;
}


bool
AREAResource::Load(TArchive *archive, uint32 key)
{
	Resource::Load(archive, key);

	char signature[5];
	signature[4] = '\0';
	ReadAt(0, signature, 4);

	if (strcmp(signature, AREA_SIGNATURE)) {
		printf("AREAResource::Load(): invalid signature %s\n", signature);
		return false;
	}

	char version[5];
	version[4] = '\0';
	ReadAt(4, version, 4);

	if (strcmp(version, AREA_VERSION_1)) {
		printf("AREAResource::Load(): version %s not supported\n", version);
		return false;
	}

	printf("AREAResource::Load(): signature %s, version %s: OK\n",
			signature, version);

	ReadAt(8, fWedName);

	ReadAt(172, fNumAnimations);
	ReadAt(176, fAnimationsOffset);

	_LoadAnimations();

	return true;
}


const char *
AREAResource::WedName() const
{
	return fWedName;
}


int32
AREAResource::CountAnimations() const
{
	return fNumAnimations;
}


animation *
AREAResource::AnimationAt(int32 i)
{
	return &fAnimations[i];
}


void
AREAResource::_LoadAnimations()
{
	fAnimations = new animation[fNumAnimations];

	Seek(fAnimationsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumAnimations; i++)
		Read(fAnimations[i]);
}
