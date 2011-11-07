#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

class ARAResource : public Resource {
public:
	//AREAResource(uint8 *data, uint32 size, uint32 key);
	ARAResource(const res_ref& name);
	~ARAResource();

	bool Load(Archive *archive, uint32 key);

	const char *WedName() const;

	int32 CountAnimations() const;
	animation *AnimationAt(int32 index);

	int16 CountActors() const;
	actor *ActorAt(int16 index);

private:

	void _LoadAnimations();
	void _LoadActors();

	res_ref fWedName;

	uint32 fAnimationsOffset;
	uint32 fNumAnimations;

	uint32 fActorsOffset;
	uint16 fNumActors;

	animation *fAnimations;
	actor *fActors;
};

#endif /* __AREARESOURCE_H_ */
