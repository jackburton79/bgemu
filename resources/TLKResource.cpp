#include "TLKResource.h"

#include "MemoryStream.h"

#include <stdlib.h>

#define TLK_SIGNATURE "TLK "
#define TLK_VERSION_1 "V1  "


enum tlk_flags {
	TLK_NO_MESSAGE_DATA = 0,
	TLK_TEXT = 1,
	TLK_SOUND = 1 << 1,
	TLK_STANDARD = 1 << 2,
	TLK_TOKEN = 1 << 3
};

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


TLKEntry*
TLKResource::EntryAt(int32 index)
{
	if (index < 0 || index >= fNumEntries)
		return NULL;

	std::cout << "tlk_entry " << index << ": " << std::endl;
	TLKEntry *newEntry = new TLKEntry;
	tlk_entry entry;
	fData->ReadAt(18 + sizeof(entry) * index, entry);
	std::cout << "flags: " << entry.flags << std::endl;
	if (entry.flags & TLK_SOUND)
		newEntry->sound_ref = entry.sound;
	if (entry.flags & TLK_TEXT)
		_ReadString(entry.string_offset, newEntry->text, entry.string_length);

	return newEntry;
}


void
TLKResource::_ReadString(int32 offset, std::string& text, int32 length)
{
	text.resize(length + 1);	
	fData->ReadAt(fDataOffset + offset, &text[0], length);
	text[length] = '\0';
}


void
TLKResource::DumpEntries()
{
	for (int32 i = 0; i < fNumEntries; i++) {
		TLKEntry *entry = EntryAt(i);
		if (entry != NULL) {
			std::cout << "Entry " << i << ":" << std::endl;
			//std::cout << "flags: " << entry->flags << std::endl;
			//if (entry->flags & TLK_TEXT)
				std::cout << entry->text << std::endl;
			delete entry;
		}
	}
}


// TLKEntry
TLKEntry::TLKEntry()
{
}


TLKEntry::~TLKEntry()
{
}

