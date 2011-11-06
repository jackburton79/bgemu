#ifndef __AREARESOURCE_H_
#define __AREARESOURCE_H_

#include "Resource.h"

class AREAResource : public Resource {
public:
	//AREAResource(uint8 *data, uint32 size, uint32 key);
	AREAResource(const res_ref& name);
	~AREAResource();

	bool Load(TArchive *archive, uint32 key);

	const char *WedName() const;

	int32 CountAnimations() const;
	animation *AnimationAt(int32 i);

private:

	void _LoadAnimations();

	res_ref fWedName;

	uint32 fAnimationsOffset;
	uint32 fNumAnimations;

	animation *fAnimations;
};

#endif /* __AREARESOURCE_H_ */
