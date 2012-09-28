#ifndef __KEYRESOURCE_H
#define __KEYRESOURCE_H

#include "Resource.h"

struct KeyFileEntry {
	uint32 length;
	uint16 location;
	char name[32];
};


struct KeyResEntry {
	res_ref name;
	uint16 type;
	uint32 key;
};

class KEYResource : public Resource {
public:
	KEYResource(const res_ref &name);
	virtual ~KEYResource();

	virtual bool Load(Archive *archive, uint32 key);

	uint32 CountFileEntries() const;
	bool GetFileEntryAt(uint32 index, KeyFileEntry &entry);

	uint32 CountResourceEntries() const;
	bool GetResEntryAt(uint32 index, KeyResEntry &entry);

private:
	uint32 fNumBifs;
	uint32 fNumResources;
	uint32 fBifOffset;
	uint32 fResOffset;
};

#endif // __KEYRESOURCE_H
