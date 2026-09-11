#include "AreaResource.h"

#include "Actor.h"
#include "Container.h"
#include "CreResource.h"
#include "FileStream.h"
#include "Log.h"
#include "MemoryStream.h"
#include "Region.h"
#include "ResManager.h"
#include "Stream.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#define AREA_SIGNATURE "AREA"
#define AREA_VERSION_1 "V1.0"


/* static */
Resource*
ARAResource::Create(const res_ref& name)
{
	return new ARAResource(name);
}


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
	fDoors(NULL),
	fContainers(NULL)
{
}


ARAResource::~ARAResource()
{
	delete[] fAnimations;
	delete[] fActors;
	delete[] fDoors;
	delete[] fRegions;
	delete[] fContainers;
}


bool
ARAResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	return _ParseData();
}


// Shared by Load() above (a real KEY/BIF-backed load) and LoadFromFile()
// below (a session checkpoint written by WriteToFile()) - both just need
// fData already pointing at a valid ARE V1.0 buffer before this runs.
bool
ARAResource::_ParseData()
{
	if (!CheckSignature(AREA_SIGNATURE))
		return false;

	if (!CheckVersion(AREA_VERSION_1))
		return false;

	fData->ReadAt(8, fWedName);

	fData->ReadAt(84, fActorsOffset);
	fData->ReadAt(88, fNumActors);

	fData->ReadAt(90, fNumRegions);
	fData->ReadAt(92, fRegionsOffset);

	fData->ReadAt(124, fVerticesOffset);

	fData->ReadAt(164, fNumDoors);
	fData->ReadAt(168, fDoorsOffset);

	fData->ReadAt(172, fNumAnimations);
	fData->ReadAt(176, fAnimationsOffset);

	_LoadAnimations();
	_LoadActors();
	_LoadDoors();
	_LoadRegions();
	_LoadContainers();

	return true;
}


bool
ARAResource::LoadFromFile(const char* path)
{
	try {
		FileStream file(path, FileStream::READ_ONLY);
		delete fData;
		fData = new MemoryStream(file.Size());
		file.ReadAt(0, fData->Data(), file.Size());
	} catch (std::exception& e) {
		std::cerr << "ARAResource::LoadFromFile(" << path << "): " << e.what() << std::endl;
		return false;
	}

	return _ParseData();
}


bool
ARAResource::WriteToFile(const char* path) const
{
	size_t size = fData->Size();
	std::vector<uint8> buffer(size);
	fData->ReadAt(0, buffer.data(), size);

	// Embedded CRE bytes (if any, see SetEmbeddedCRE()) are appended
	// after the file's own original bytes, so every other section keeps
	// the exact offset _ParseData() (and every other reader) already
	// expects - only each patched actor's own cre_offset/cre_size point
	// into this new tail.
	size_t appendOffset = size;
	std::vector<uint8> appendedData;

	for (uint32 i = 0; i < fNumActors; i++) {
		size_t offset = fActorsOffset + i * sizeof(IE::actor);
		if (offset + sizeof(IE::actor) > size)
			continue;

		IE::actor entry = fActors[i];
		auto pending = fPendingEmbeddedCRE.find((uint16)i);
		if (pending != fPendingEmbeddedCRE.end() && !pending->second.empty()) {
			const std::vector<uint8>& creData = pending->second;
			entry.flags &= ~(uint32)IE::ACTOR_CRE_EXTERNAL;
			entry.cre_offset = (uint32)appendOffset;
			entry.cre_size = (uint32)creData.size();
			appendedData.insert(appendedData.end(), creData.begin(), creData.end());
			appendOffset += creData.size();
		}
		memcpy(buffer.data() + offset, &entry, sizeof(IE::actor));
	}
	for (uint32 i = 0; i < fNumDoors; i++) {
		size_t offset = fDoorsOffset + i * sizeof(IE::door);
		if (offset + sizeof(IE::door) <= size)
			memcpy(buffer.data() + offset, &fDoors[i], sizeof(IE::door));
	}
	// Consumed - see fPendingEmbeddedCRE's own comment on why this
	// can't just accumulate across multiple checkpoint writes.
	fPendingEmbeddedCRE.clear();

	if (!appendedData.empty())
		buffer.insert(buffer.end(), appendedData.begin(), appendedData.end());

	try {
		FileStream file(path, FileStream::WRITE_ONLY | FileStream::CREATE);
		file.Write(buffer.data(), buffer.size());
	} catch (std::exception& e) {
		std::cerr << "ARAResource::WriteToFile(" << path << "): " << e.what() << std::endl;
		return false;
	}
	return true;
}


int32
ARAResource::IndexOfActorEntry(const IE::actor* entry) const
{
	if (entry < fActors || entry >= fActors + fNumActors)
		return -1;
	return (int32)(entry - fActors);
}


void
ARAResource::SetEmbeddedCRE(uint16 index, const std::vector<uint8>& creData)
{
	if (index < fNumActors)
		fPendingEmbeddedCRE[index] = creData;
}


const res_ref&
ARAResource::WedName() const
{
	return fWedName;
}


uint16
ARAResource::Flags() const
{
	uint16 type;
	fData->ReadAt(0x48, type);
	return type;
}


uint32
ARAResource::CountDoors() const
{
	return fNumDoors;
}


IE::door*
ARAResource::DoorAt(uint32 index)
{
	if (index >= fNumDoors) {
		std::cerr << Log::Red << "ARAResource::DoorAt(): Requested wrong door" << std::endl;
		return NULL;
	}

	return &fDoors[index];
}


IE::point
ARAResource::VertexAt(uint32 index)
{
	IE::point vertex = { 0, 0 };
	fData->ReadAt(fVerticesOffset + index * sizeof(IE::point), vertex);
	return vertex;
}


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
	if (index >= fNumActors) {
		std::cerr << Log::Red << "ARAResource::GetActorAt(): Requested wrong actor" << std::endl;
		return NULL;
	}

	// TODO: No need to preload the fActor array.
	// We can load actor by actor from here.
	IE::actor& ieActor = fActors[index];

	// CRE-attached flag clear = embedded (see WriteToFile()'s
	// SetEmbeddedCRE() side, and the IESDP are_v1 comment on this bit) -
	// this actor's own previously-checkpointed state (HP, inventory,
	// spellbook, ...), not the pristine KEY/BIF template the Actor(
	// IE::actor&) constructor below would otherwise load by resref
	// (ieActor.cre). cre_offset is an absolute offset into this same ARE
	// resource's data (confirmed against a real IE engine's own ARE
	// writer - not relative to the actor table or this entry, unlike an
	// earlier, never-completed attempt at this).
	CREResource* cre = NULL;
	if ((ieActor.flags & IE::ACTOR_CRE_EXTERNAL) == 0 && ieActor.cre_size > 0) {
		try {
			std::vector<uint8> creData(ieActor.cre_size);
			fData->ReadAt(ieActor.cre_offset, creData.data(), ieActor.cre_size);
			MemoryStream stream(creData.data(), creData.size(), false);
			cre = new CREResource(ieActor.cre);
			cre->Acquire(); // Resource starts at refcount 0
			cre->Load(&stream, 0, (uint32)creData.size());
			cre->Init();
		} catch (std::exception& e) {
			std::cerr << Log::Red << "ARAResource::GetActorAt(): failed to load "
				"embedded CRE for actor " << index << " (" << ieActor.name
				<< "): " << e.what() << Log::Normal << std::endl;
			if (cre != NULL) {
				gResManager->ReleaseResource(cre);
				cre = NULL;
			}
		}
	}

	ieActor.Print();
	return cre != NULL ? new Actor(ieActor, cre) : new Actor(ieActor);
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
	::Polygon& polygon = const_cast<Polygon&>(region->Polygon());

	uint32 verticesOffset;
	fData->ReadAt(0x007c, verticesOffset);
	fData->Seek(verticesOffset + fRegions[index].vertex_index * sizeof(IE::point), SEEK_SET);
	for (uint16 v = 0; v < fRegions[index].vertex_count; v++) {
		IE::point vertex;
		fData->Read(vertex);
		polygon.AddPoint(vertex.x, vertex.y);
	}

	//fRegions[index].Print();
	return region;
}


uint16
ARAResource::CountContainers() const
{
	uint16 count;
	fData->ReadAt(0x74, count);
	return count;
}


Container*
ARAResource::GetContainerAt(uint16 index)
{
	if (index >= CountContainers())
		return NULL;

	Container* container = new Container(&fContainers[index]);
	::Polygon& polygon = const_cast<Polygon&>(container->Polygon());
	polygon.SetFrame(fContainers[index].x_min,
				fContainers[index].x_max,
				fContainers[index].y_min,
				fContainers[index].y_max);

	uint32 verticesOffset;
	fData->ReadAt(0x007c, verticesOffset);
	fData->Seek(verticesOffset + fContainers[index].vertex_first_index * sizeof(IE::point), SEEK_SET);
	for (uint16 v = 0; v < fContainers[index].vertices_count; v++) {
		IE::point vertex;
		fData->Read(vertex);
		polygon.AddPoint(vertex.x, vertex.y);
	}

	// The container's items live in the area's own shared item list
	// (0x0078 "Offset to items" - same list containers/piles/etc. all
	// slice into by index), not inside the container structure itself -
	// same "index into a shared table" shape as CRE's Items table.
	uint32 itemsOffset;
	fData->ReadAt(0x0078, itemsOffset);
	fData->Seek(itemsOffset + fContainers[index].item_first_index * sizeof(IE::item), SEEK_SET);
	for (uint32 i = 0; i < fContainers[index].item_count; i++) {
		IE::item item;
		fData->Read(item);
		container->AddContainerItem(item);
	}

	return container;
}


uint32
ARAResource::CountEntrances()
{
	uint32 numEntrances = 0;
	fData->ReadAt(0x006c, numEntrances);
	return numEntrances;
}


IE::entrance
ARAResource::EntranceAt(uint32 index)
{
	if (index >= CountEntrances())
		throw std::out_of_range("ARAResource::EntranceAt()");

	uint32 entranceOffset;
	fData->ReadAt(0x0068, entranceOffset);
	IE::entrance entrance;
	fData->ReadAt(entranceOffset + sizeof(entrance) * index, entrance);
	return entrance;
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
		//fActors[i].Print();
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
		std::cout << "Door " << fDoors[i].name << std::endl;
		for (uint16 c = 0; c < fDoors[i].closed_vertices_count; c++) {
			IE::point vertex;
			fData->ReadAt(0x007c + (c + fDoors[i].closed_vertex_index) * sizeof(IE::point), vertex);
			closedPolygon.AddPoint(vertex.x, vertex.y);
		}

		//closedPolygon.Print();

		Polygon openPolygon;
		for (uint16 c = 0; c < fDoors[i].open_vertices_count; c++) {
			IE::point vertex;
			fData->ReadAt(0x007c + (c + fDoors[i].open_vertex_index) * sizeof(IE::point), vertex);
			openPolygon.AddPoint(vertex.x, vertex.y);
		}
		//openPolygon.Print();
	}
}


void
ARAResource::_LoadContainers()
{
	uint16 count;
	fData->ReadAt(0x74, count);
	uint32 offset;
	fData->ReadAt(0x70, offset);

	std::cout << count << " containers at offset " << offset << std::endl;
	fData->Seek(offset, SEEK_SET);
	fContainers = new IE::container[count];
	for (uint16 i = 0; i < count; i++) {
		fData->Read(fContainers[i]);
	}
}


void
ARAResource::_LoadRegions()
{
	fRegions = new IE::region[fNumRegions];
	fData->Seek(fRegionsOffset, SEEK_SET);
	for (uint32 i = 0; i < fNumRegions; i++)
		fData->Read(fRegions[i]);
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
