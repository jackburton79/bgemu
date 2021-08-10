#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

class Actor;
class Container;
class Door;
class Region;
class ARAResource : public Resource {
public:
	ARAResource(const res_ref& name);

	virtual bool Load(Archive *archive, uint32 key);

	const res_ref& WedName() const;
	uint16 Flags() const;

	res_ref NorthAreaName();
	res_ref EastAreaName();
	res_ref SouthAreaName();
	res_ref WestAreaName();

	uint32 CountDoors() const;
	IE::door *DoorAt(uint32 index);

	uint32 CountAnimations() const;
	IE::animation *AnimationAt(uint32 index);

	uint16 CountActors() const;
	Actor* GetActorAt(uint16 index);

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
};

#endif /* __AREARESOURCE_H_ */
