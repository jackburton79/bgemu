#include "AreaResource.h"

#include "Actor.h"
#include "CreResource.h"
#include "Door.h"
#include "MemoryStream.h"
#include "Region.h"
#include "ResManager.h"

#define AREA_SIGNATURE "AREA"
#define AREA_VERSION_1 "V1.0"



ARAResource::ARAResource(const res_ref& name)
	:
	Resource(name, RES_ARA),
	fWedName(NULL),
	fActorsOffset(0),
	fNumActors(0),
	fRegionsOffset(0),
	fNumRegions(0),
	fAnimationsOffset(0),
	fNumAnimations(0),
	fNumDoors(0),
	fDoorsOffset(0),
	fVerticesOffset(0),
	fAnimations(NULL),
	fActors(NULL),
	fRegions(NULL),
	fDoors(NULL)
{
}


ARAResource::~ARAResource()
{
	delete[] fAnimations;
	delete[] fActors;
	delete[] fDoors;
	delete[] fRegions;
}


bool
ARAResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(AREA_SIGNATURE))
		return false;

	if (!CheckVersion(AREA_VERSION_1))
		return false;

	fData->ReadAt(8, fWedName);

	fData->ReadAt(84, fActorsOffset);
	fData->ReadAt(88, fNumActors);

	fData->ReadAt(90, fNumRegions);
	fData->ReadAt(92, fRegionsOffset);

	fData->ReadAt(164, fNumDoors);
	fData->ReadAt(168, fDoorsOffset);

	fData->ReadAt(172, fNumAnimations);
	fData->ReadAt(176, fAnimationsOffset);


	fData->ReadAt(0x7c, fVerticesOffset);

	_LoadAnimations();
	_LoadActors();
	_LoadDoors();
	_LoadRegions();

	return true;
}


const res_ref&
ARAResource::WedName() const
{
	return fWedName;
}


uint32
ARAResource::CountDoors() const
{
	return fNumDoors;
}


IE::door *
ARAResource::DoorAt(uint32 index)
{
	if (index > fNumDoors) {
		printf("Requested wrong door.\n");
		return NULL;
	}

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


IE::animation*
ARAResource::AnimationAt(uint32 index)
{
	return &fAnimations[index];
}


Actor*
ARAResource::GetActorAt(uint16 index)
{
	// TODO: No need to preload the fActor array.
	// We can load actor by actor from here.
	IE::actor& ieActor = fActors[index];
	Actor* newActor = NULL;
	if (ieActor.flags & IE::ACTOR_CRE_EXTERNAL) {

	} else {
		//CREResource* cre = new CREResource(ieActor.cre);
		fData->Seek(fActorsOffset + ieActor.cre_offset
						+ sizeof(IE::actor) * index, SEEK_SET);
		char array[64];
		std::cout << "Embedded CRE (" << ieActor.cre_size << ")" << std::endl;
		fData->Read(array);
		for (int32 i = 0; i < 64; i++)
			std::cout << array[i] << std::endl;

		//newActor = new Actor(ieActor, cre);
	}

	newActor = new Actor(ieActor);
	return newActor;
}


uint16
ARAResource::CountActors() const
{
	return fNumActors;
}


uint16
ARAResource::CountRegions() const
{
	return fNumRegions;
}


Region*
ARAResource::GetRegionAt(uint16 index)
{
	Region* region = new Region(&fRegions[index]);

	return region;
}


uint32
ARAResource::CountVariables() const
{
	uint32 numVars = 0;
	fData->ReadAt(0x8c, numVars);
	return numVars;
}


IE::variable
ARAResource::VariableAt(uint32 index)
{
	IE::variable var;
	uint32 offset;
	fData->ReadAt(0x88, offset);
	fData->ReadAt(offset + index * sizeof(IE::variable), var);
	return var;
}


res_ref
ARAResource::ScriptName()
{
	res_ref script;
	fData->ReadAt(0x94, script);
	return script;
}


void
ARAResource::_LoadAnimations()
{
	fAnimations = new IE::animation[fNumAnimations];

	fData->Seek(fAnimationsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumAnimations; i++)
		fData->Read(fAnimations[i]);
}


void
ARAResource::_LoadActors()
{
	// TODO: No need to preload the fActors array.
	// also check the TODO in GetActorAt()
	fActors = new IE::actor[fNumActors];

	fData->Seek(fActorsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumActors; i++) {
		fData->Read(fActors[i]);
		if (fActors[i].flags & IE::ACTOR_CRE_EXTERNAL) {
			char c;
			fData->ReadAt(fData->Position() + fActors[i].cre_offset, c);
			//std::cout << "attached data: " << c << std::endl;
		}

	}
}


void
ARAResource::_LoadDoors()
{
	fDoors = new IE::door[fNumDoors];
	fData->Seek(fDoorsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumDoors; i++) {
		fData->Read(fDoors[i]);
		Polygon closedPolygon;
		for (uint16 c = 0; c < fDoors[i].closed_vertices_count; c++) {
			IE::point vertex;
			fData->ReadAt(0x007c + fDoors[i].closed_vertex_index, vertex);
			closedPolygon.AddPoints(&vertex, 1);
		}
		Polygon openPolygon;
		for (uint16 c = 0; c < fDoors[i].open_vertex_index; c++) {
			IE::point vertex;
			fData->ReadAt(0x007c + fDoors[i].open_vertex_index, vertex);
			openPolygon.AddPoints(&vertex, 1);
		}
	}
}


void
ARAResource::_LoadRegions()
{
	fRegions = new IE::region[fNumRegions];
	fData->Seek(fRegionsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumRegions; i++) {
		fData->Read(fRegions[i]);
		fRegions[i].Print();
	}
}


res_ref
ARAResource::NorthAreaName()
{
	res_ref name;
	fData->ReadAt(0x18, name);
	return name;
}


res_ref
ARAResource::EastAreaName()
{
	res_ref name;
	fData->ReadAt(0x24, name);
	return name;
}


res_ref
ARAResource::SouthAreaName()
{
	res_ref name;
	fData->ReadAt(0x30, name);
	return name;
}


res_ref
ARAResource::WestAreaName()
{
	res_ref name;
	fData->ReadAt(0x3c, name);
	return name;
}
