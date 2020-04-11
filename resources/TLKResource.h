#ifndef __TLKRESOURCE_H
#define __TLKRESOURCE_H

#include "Resource.h"

#include <string>

class TLKEntry {
public:
	TLKEntry();
	~TLKEntry();
	std::string text;
	res_ref sound_ref;
};


class TLKResource : public Resource {
public:
	TLKResource(const res_ref &name);

	virtual bool Load(Archive *archive, uint32 key);

	int32 CountEntries();
	TLKEntry *EntryAt(int32 e);

	void DumpEntries();

private:
	virtual ~TLKResource();

	void _ReadString(int32 offset, std::string &text, int32 length);

	int32 fNumEntries;
	uint32 fDataOffset;
};

#endif // __TLKRESOURCE_H
