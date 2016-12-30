#include "KEYResource.h"
#include "MemoryStream.h"
#include "Utils.h"

#define KEY_SIGNATURE "KEY "
#define KEY_VERSION "V1  "

const static int32 kKeyFileEntrySize = 12;
const static int32 kKeyResEntrySize = 14;

void
KeyFileEntry::Dump() const
{
	std::cout << name << ": length " << length;
	std::cout << ", location " << location << std::endl;
}


KEYResource::KEYResource(const res_ref &name)
	:
	Resource(name, 0),
	fNumBifs(0),
	fNumResources(0),
	fBifOffset(0),
	fResOffset(0)
{
}


/* virtual */
KEYResource::~KEYResource()
{

}


/* virtual */
bool
KEYResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(KEY_SIGNATURE))
		return false;

	if (!CheckVersion(KEY_VERSION))
		return false;

	fData->Seek(8, SEEK_SET);
	fData->Read(fNumBifs);
	fData->Read(fNumResources);
	fData->Read(fBifOffset);
	fData->Read(fResOffset);

	return true;
}


uint32
KEYResource::CountFileEntries() const
{
	return fNumBifs;
}


KeyFileEntry*
KEYResource::GetFileEntryAt(uint32 index)
{
	KeyFileEntry* entry = NULL;
	try {
		entry = new KeyFileEntry;
		fData->Seek(fBifOffset + index * kKeyFileEntrySize, SEEK_SET);
		fData->Read(entry->length);
		uint32 offset;
		fData->Read(offset);
		uint16 nameLen;
		fData->Read(nameLen);
		fData->Read(entry->location);
		entry->name = new char[nameLen + 1];
		fData->ReadAt(offset, entry->name, nameLen);
		path_dos_to_unix(entry->name);
	} catch (...) {
		delete entry;
		entry = NULL;
	}

	return entry;
}


uint32
KEYResource::CountResourceEntries() const
{
	return fNumResources;
}


KeyResEntry*
KEYResource::GetResEntryAt(uint32 index)
{
	KeyResEntry* entry = NULL;
	try {
		entry = new KeyResEntry;
		fData->Seek(fResOffset + index * kKeyResEntrySize, SEEK_SET);
		fData->Read(entry->name);
		fData->Read(entry->type);
		fData->Read(entry->key);
		if (!strcmp(entry->name.name, "")) {
			//std::cerr << "BUG: unnamed resource at index ";
			//std::cerr << index << "!!!" << std::endl;
			// TODO: looks like in BG2 there is an unnamed resource
			// and this causes all kinds of problems. Investigate
			throw -1;
		}
	} catch (...) {
		delete entry;
		entry = NULL;
	}
	return entry;
}


/* virtual */
void
KEYResource::Dump()
{
	std::cout << "KeyResource " << Name() << std::endl;
	std::cout << "\tNumber of BIF files: " << CountFileEntries() << std::endl;
	std::cout << "\tBIFs offset: " << fBifOffset << std::endl;
	std::cout << "\tNumber of Resources: " << CountResourceEntries() << std::endl;
	std::cout << "\tResources offset: " << fResOffset << std::endl;
	std::cout << "BIF entries: " << std::endl;
	for (uint32 i = 0; i < CountFileEntries(); i++) {
		KeyFileEntry* fileEntry = GetFileEntryAt(i);
		if (fileEntry != NULL) {
			fileEntry->Dump();
			delete fileEntry;
		}
	}
	std::cout << "Resource entries: " << std::endl;
	for (uint32 i = 0; i < CountResourceEntries(); i++) {
		KeyResEntry* resEntry = GetResEntryAt(i);
		if (resEntry != NULL) {
			//resEntry->Dump();
			delete resEntry;
		}
	}
}


/* virtual */
void
KEYResource::LastReferenceReleased()
{
	delete this;
}
