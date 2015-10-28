#ifndef __BCSRESOURCE_H
#define __BCSRESOURCE_H

#include "Resource.h"

class Script;
class BCSResource : public Resource {
public:
	BCSResource(const res_ref &name);

	virtual bool Load(Archive *archive, uint32 key);

	Script *GetScript() const;
private:
	virtual ~BCSResource();
};

#endif // __BCSRESOURCE_H
