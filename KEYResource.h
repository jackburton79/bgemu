#ifndef __KEYRESOURCE_H
#define __KEYRESOURCE_H

#include "Resource.h"

#define RES_BIF_FILE_INDEX_MASK 0x00003FFF
#define RES_TILESET_INDEX_MASK 0x000FC000

#define RES_BIF_INDEX(x) ((x) >> 20)
#define RES_BIF_FILE_INDEX(x) ((x) & RES_BIF_FILE_INDEX_MASK)
#define RES_TILESET_INDEX(x) ((((x) & RES_TILESET_INDEX_MASK) >> 14) - 1)

struct KeyFileEntry {
	uint32 length;
	uint16 location;
	std::string name;
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
