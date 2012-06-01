#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

class Door;
class ARAResource : public Resource {
public:
	ARAResource(const res_ref& name);
	~ARAResource();

	bool Load(Archive *archive, uint32 key);

	const char *WedName() const;

	uint32 CountDoors() const;
	IE::door *DoorAt(uint32 index);

	uint32 CountAnimations() const;
	IE::animation *AnimationAt(uint32 index);

	uint16 CountActors() const;
	IE::actor *ActorAt(uint16 index);

	res_ref ScriptName();

	uint32 CountVariables() const;
	IE::variable VariableAt(uint32 index);

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

	uint32 fVerticesOffset;

	IE::animation *fAnimations;
	IE::actor *fActors;
	IE::door *fDoors;
};

#endif /* __AREARESOURCE_H_ */
