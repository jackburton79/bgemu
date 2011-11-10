#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

class ARAResource : public Resource {
public:
	ARAResource(const res_ref& name);
	~ARAResource();

	bool Load(Archive *archive, uint32 key);

	const char *WedName() const;

	uint32 CountDoors() const;
	door *DoorAt(uint32 index);

	uint32 CountAnimations() const;
	animation *AnimationAt(uint32 index);

	uint16 CountActors() const;
	actor *ActorAt(uint16 index);

private:

	void _LoadAnimations();
	void _LoadActors();
	void _LoadDoors();

	res_ref fWedName;

	uint32 fActorsOffset;
	uint16 fNumActors;

	uint32 fAnimationsOffset;
	uint32 fNumAnimations;

	uint32 fNumDoors;
	uint32 fDoorsOffset;

	animation *fAnimations;
	actor *fActors;
	door *fDoors;
};

#endif /* __AREARESOURCE_H_ */
