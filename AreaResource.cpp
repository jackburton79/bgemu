#include "AreaResource.h"
#include "MemoryStream.h"

#define AREA_SIGNATURE "AREA"
#define AREA_VERSION_1 "V1.0"

static void
PrintActor(const ::actor &actor)
{
	printf("Actor %s:\n", actor.name);
	printf("\tCRE: %s\n", (const char *)actor.cre);
	printf("\tposition: (%d, %d)\n", actor.position.x, actor.position.y);
}


ARAResource::ARAResource(const res_ref& name)
	:
	Resource(name, RES_ARA),
	fWedName(NULL),
	fAnimationsOffset(0),
	fNumAnimations(0),
	fActorsOffset(0),
	fNumActors(0),
	fAnimations(NULL),
	fActors(NULL)
{
}


ARAResource::~ARAResource()
{
	delete[] fAnimations;
	delete[] fActors;
}


bool
ARAResource::Load(Archive *archive, uint32 key)
{
	Resource::Load(archive, key);

	if (!CheckSignature(AREA_SIGNATURE))
		return false;

	if (!CheckVersion(AREA_VERSION_1))
		return false;

	fData->ReadAt(8, fWedName);

	fData->ReadAt(84, fActorsOffset);
	fData->ReadAt(88, fNumActors);

	fData->ReadAt(172, fNumAnimations);
	fData->ReadAt(176, fAnimationsOffset);

	_LoadAnimations();
	_LoadActors();

	DropData();

	return true;
}


const char *
ARAResource::WedName() const
{
	return fWedName;
}


int32
ARAResource::CountAnimations() const
{
	return fNumAnimations;
}


animation *
ARAResource::AnimationAt(int32 index)
{
	return &fAnimations[index];
}


actor *
ARAResource::ActorAt(int16 index)
{
	return &fActors[index];
}


int16
ARAResource::CountActors() const
{
	return fNumActors;
}


void
ARAResource::_LoadAnimations()
{
	fAnimations = new animation[fNumAnimations];

	fData->Seek(fAnimationsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumAnimations; i++)
		fData->Read(fAnimations[i]);
}


void
ARAResource::_LoadActors()
{
	fActors = new actor[fNumActors];

	fData->Seek(fActorsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumActors; i++) {
		fData->Read(fActors[i]);
		//PrintActor(fActors[i]);
	}
}
