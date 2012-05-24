#include "AreaResource.h"
#include "Door.h"
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
	fActorsOffset(0),
	fNumActors(0),
	fAnimationsOffset(0),
	fNumAnimations(0),
	fNumDoors(0),
	fDoorsOffset(0),
	fAnimations(NULL),
	fActors(NULL),
	fDoors(NULL)
{
}


ARAResource::~ARAResource()
{
	delete[] fAnimations;
	delete[] fActors;
	delete[] fDoors;
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

	fData->ReadAt(164, fNumDoors);
	fData->ReadAt(168, fDoorsOffset);

	fData->ReadAt(172, fNumAnimations);
	fData->ReadAt(176, fAnimationsOffset);

	fData->ReadAt(0x7c, fVerticesOffset);

	_LoadAnimations();
	_LoadActors();
	_LoadDoors();

	return true;
}


const char *
ARAResource::WedName() const
{
	return fWedName;
}


uint32
ARAResource::CountDoors() const
{
	return fNumDoors;
}


door *
ARAResource::DoorAt(uint32 index)
{
	return &fDoors[index];
}

/*
Door *
ARAResource::GetDoor(uint32 index)
{
	Door *d = new Door();

	uint16 tileIndex = fDoors[index].open_cell_index;
	for (uint32 i = 0; i < fDoors[index].open_cell_count; i++) {
		d->fTilesOpen.push_back(tileIndex++);
	}

	printf("index: %d count: %d\n", fDoors[index].open_cell_index,
			fDoors[index].open_cell_count);

	tileIndex = fDoors[index].closed_cell_index;
	for (uint32 i = 0; i < fDoors[index].closed_cell_count; i++) {
		d->fTilesClosed.push_back(tileIndex++);
	}

	return d;
}*/


uint32
ARAResource::CountAnimations() const
{
	return fNumAnimations;
}


animation *
ARAResource::AnimationAt(uint32 index)
{
	return &fAnimations[index];
}


actor *
ARAResource::ActorAt(uint16 index)
{
	return &fActors[index];
}


uint16
ARAResource::CountActors() const
{
	return fNumActors;
}


uint32
ARAResource::CountVariables() const
{
	uint32 numVars = 0;
	fData->ReadAt(0x8c, numVars);
	return numVars;
}


variable
ARAResource::VariableAt(uint32 index)
{
	variable var;
	uint32 offset;
	fData->ReadAt(0x88, offset);
	fData->ReadAt(offset + index * sizeof(variable), var);
	return var;
}


res_ref
ARAResource::Script()
{
	res_ref script;
	fData->ReadAt(0x94, script);
	return script;
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
	}
}


void
ARAResource::_LoadDoors()
{
	fDoors = new door[fNumDoors];
	fData->Seek(fDoorsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumDoors; i++) {
		fData->Read(fDoors[i]);
	}
}
