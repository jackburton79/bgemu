#ifndef __KEYRESOURCE_H
#define __KEYRESOURCE_H

#include "Resource.h"

struct KeyFileEntry {
	KeyFileEntry() { name = NULL; };
	~KeyFileEntry() { delete[] name; };

	void Dump() const;
	
	uint32 length;
	uint16 location;
	char* name;

private:
	KeyFileEntry& operator=(const KeyFileEntry&);
	KeyFileEntry(const KeyFileEntry&);
};


struct KeyResEntry {
	res_ref name;
	uint16 type;
	uint32 key;
};

class KEYResource : public Resource {
public:
	KEYResource(const res_ref &name);

	virtual bool Load(Archive *archive, uint32 key);

	uint32 CountFileEntries() const;
	KeyFileEntry* GetFileEntryAt(uint32 index);

	uint32 CountResourceEntries() const;
	KeyResEntry* GetResEntryAt(uint32 index);

	virtual void Dump();
	
private:
	virtual ~KEYResource();

	virtual void LastReferenceReleased();

	uint32 fNumBifs;
	uint32 fNumResources;
	uint32 fBifOffset;
	uint32 fResOffset;
};

#endif // __KEYRESOURCE_H
