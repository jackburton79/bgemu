#include "KEYResource.h"
#include "MemoryStream.h"
#include "Utils.h"

#define KEY_SIGNATURE "KEY "
#define KEY_VERSION "V1  "

const static int32 kKeyFileEntrySize = 12;
const static int32 kKeyResEntrySize = 14;

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
		int32 offset;
		int16 nameLen;

		fData->Read(entry->length);
		fData->Read(offset);
		fData->Read(nameLen);
		fData->Read(entry->location);
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
