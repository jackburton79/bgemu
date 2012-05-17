#ifndef __BCSRESOURCE_H
#define __BCSRESOURCE_H

#include "Resource.h"

struct script;
class BCSResource : public Resource {
public:
	BCSResource(const res_ref &name);
	~BCSResource();

	bool Load(Archive *archive, uint32 key);
	void RunScript();

private:
	void _LoadScript();
};

#endif // __BCSRESOURCE_H
