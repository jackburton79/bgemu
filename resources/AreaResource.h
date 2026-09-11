#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

#include <map>
#include <vector>

class Actor;
class Container;
class Door;
class Region;
class ARAResource : public Resource {
public:
	ARAResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	// Session-checkpoint persistence (see Game::AreaCache's own comment
	// for why this exists) - not a KEY/BIF-backed load, a plain file
	// previously produced by this same WriteToFile(). Shares every table
	// parser with Load() above (see the private _ParseData()); the only
	// difference is where fData itself comes from.
	bool LoadFromFile(const char* path);

	// Writes this area back out, byte-identical to what was loaded
	// except for the actor and door tables (see their own comment) -
	// the only two tables anything in this engine actually mutates at
	// runtime (Actor::fActor/Door::fAreaDoor alias fActors[]/fDoors[]
	// directly, and every setter writes straight into those in-memory
	// structs, never through fData). Everything else (variables,
	// entrances, containers+items, vertices, WED name, area flags, ...)
	// is copied verbatim from the original file's own bytes - same
	// offsets, same layout, so no full from-scratch ARE reserializer is
	// needed. Sections this engine's own Load() never reads at all
	// (spawn points, ambients, automap notes, rest interruptions, songs,
	// projectile traps, explored bitmask, tiled objects) ride along
	// unread and unchanged inside that same verbatim copy - nothing here
	// needs to understand them to preserve them. The one exception is
	// embedded CRE data: any actor registered via SetEmbeddedCRE() below
	// gets its current CRE bytes appended after the file's own original
	// bytes and its actor-table entry patched (CRE-attached flag
	// cleared, offset/size pointed at that new tail) to reference it -
	// this is what lets a non-party actor's HP/inventory/spellbook
	// survive a process restart, not just staying in this same session's
	// in-memory AreaCache (see its own comment).
	bool WriteToFile(const char* path) const;

	const res_ref& WedName() const;
	uint16 Flags() const;

	res_ref NorthAreaName();
	res_ref EastAreaName();
	res_ref SouthAreaName();
	res_ref WestAreaName();

	uint32 CountDoors() const;
	IE::door *DoorAt(uint32 index);

	// The area's shared vertex table (doors' outline/impeded-cell-block
	// data, regions, animations, etc. all index into this same table by
	// a start index + count - see e.g. IE::door's open_cell_index or
	// resources/AreaResource.cpp's _LoadDoors()). index is an absolute
	// index into the table, not relative to any one door/region.
	IE::point VertexAt(uint32 index);

	uint32 CountAnimations() const;
	IE::animation *AnimationAt(uint32 index);

	uint16 CountActors() const;
	Actor* GetActorAt(uint16 index);

	// Array index of `entry` within fActors[], or -1 if it doesn't
	// belong to this area's own actor table (e.g. a party member, or
	// one spawned at runtime - see Actor::AreaActorEntry()'s comment).
	int32 IndexOfActorEntry(const IE::actor* entry) const;

	// Registers `creData` (see Resource::RawData()) as the embedded CRE
	// to write for the actor at `index` on the next WriteToFile() -
	// AreaRoom::_UnloadArea() calls this, right before checkpointing,
	// for every still-alive non-party actor so a process restart
	// doesn't lose their current HP/inventory/spellbook/... the way a
	// pristine re-parse of the original KEY/BIF CRE would (see
	// WriteToFile()'s own comment on why the checkpoint otherwise only
	// covers what already lives directly in the ARE actor/door structs).
	void SetEmbeddedCRE(uint16 index, const std::vector<uint8>& creData);

	uint16 CountRegions() const;
	Region* GetRegionAt(uint16 index);

	uint16 CountContainers() const;
	Container* GetContainerAt(uint16 index);

	uint32 CountEntrances();
	IE::entrance EntranceAt(uint32 index);

	res_ref ScriptName();

	uint32 CountVariables() const;
	IE::variable VariableAt(uint32 index);

	static Resource* Create(const res_ref& name);

private:
	virtual ~ARAResource();
	bool _ParseData();
	void _LoadActors();
	void _LoadAnimations();
	void _LoadDoors();
	void _LoadContainers();
	void _LoadRegions();

	res_ref fWedName;

	uint32 fActorsOffset;
	uint16 fNumActors;

	uint32 fRegionsOffset;
	uint16 fNumRegions;

	uint32 fAnimationsOffset;
	uint32 fNumAnimations;

	uint32 fNumDoors;
	uint32 fDoorsOffset;

	uint32 fVerticesOffset;

	IE::animation* fAnimations;
	IE::actor* fActors;
	IE::region* fRegions;
	IE::door* fDoors;
	IE::container* fContainers;

	// Pending SetEmbeddedCRE() calls, consumed (and cleared) by the next
	// WriteToFile() - mutable since that's otherwise a const method;
	// cleared there (not just after use) so an actor that stops being
	// registered (e.g. destroyed between two checkpoints) doesn't keep
	// getting re-embedded with stale, pre-destruction data forever.
	mutable std::map<uint16, std::vector<uint8>> fPendingEmbeddedCRE;
};

#endif /* __AREARESOURCE_H_ */
