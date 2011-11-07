#include "TLKResource.h"

#include "MemoryStream.h"

#include <stdlib.h>

#define TLK_SIGNATURE "TLK "
#define TLK_VERSION_1 "V1  "

struct tlk_entry {
	uint16 flags;
	res_ref sound;
	uint32 volume_variance;
	uint32 pitch_variance;
	uint32 string_offset;
	uint32 string_length;
} __attribute__((packed));


TLKResource::TLKResource(const res_ref &name)
	:
	Resource(name, 0),
	fNumEntries(0),
	fDataOffset(0)
{
}


TLKResource::~TLKResource()
{
}


/* virtual */
bool
TLKResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(TLK_SIGNATURE))
		return false;

	if (!CheckVersion(TLK_VERSION_1))
		return false;

	fData->ReadAt(10, fNumEntries);
	fData->ReadAt(14, fDataOffset);

	return true;
}


int32
TLKResource::CountEntries()
{
	return fNumEntries;
}


TLKEntry *
TLKResource::EntryAt(int32 index)
{
	if (index < 0 || index >= fNumEntries)
		return NULL;

	TLKEntry *newEntry = new TLKEntry;
	tlk_entry entry;
	fData->ReadAt(18 + sizeof(entry) * index, entry);
	if (entry.string_length > 0)
		_ReadString(entry.string_offset, (char **)&newEntry->string, entry.string_length);

	return newEntry;
}


void
TLKResource::_ReadString(int32 offset, char **_string, int32 length)
{
	char *&string = *_string;
	string = (char*)malloc(length + 1);
	fData->ReadAt(fDataOffset + offset, string, length);
	string[length] = '\0';
}


void
TLKResource::DumpEntries()
{
	for (int32 i = 0; i < fNumEntries; i++) {
		TLKEntry *entry = EntryAt(i);
		if (entry != NULL) {
			printf("entry %d: %s\n", i, entry->string);
			delete entry;
		}
	}
}


// TLKEntry
TLKEntry::TLKEntry()
	:
	string(NULL)
{
}


TLKEntry::~TLKEntry()
{
	free((char *)string);
}

