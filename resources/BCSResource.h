#ifndef __BCSRESOURCE_H
#define __BCSRESOURCE_H

#include "Resource.h"

class Script;
class BCSResource : public Resource {
public:
	BCSResource(const res_ref &name);
	~BCSResource();

	virtual bool Load(Archive *archive, uint32 key);

	Script *GetScript();

private:
	Script* fScript;
};

#endif // __BCSRESOURCE_H
