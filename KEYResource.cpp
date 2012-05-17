#include "KEYResource.h"
#include "MemoryStream.h"
#include "Utils.h"

#define KEY_SIGNATURE "KEY "
#define KEY_VERSION "V1  "

const static int32 kKeyFileEntrySize = 12;
const static int32 kKeyResEntrySize = 14;

KEYResource::KEYResource(const res_ref &name)
	:
	Resource(name, 0)
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


bool
KEYResource::GetFileEntryAt(uint32 index, KeyFileEntry &entry)
{
	fData->Seek(fBifOffset + index * kKeyFileEntrySize, SEEK_SET);

	KeyFileEntry *bif = &entry;
	int32 offset;
	int16 nameLen;

	fData->Read(bif->length);
	fData->Read(offset);
	fData->Read(nameLen);
	fData->Read(bif->location);

	char name[nameLen];
	fData->ReadAt(offset, name, nameLen);

	path_dos_to_unix(name);

	bif->name = name;

	return true;
}


uint32
KEYResource::CountResourceEntries() const
{
	return fNumResources;
}


bool
KEYResource::GetResEntryAt(uint32 index, KeyResEntry &entry)
{
	fData->Seek(fResOffset + index * kKeyResEntrySize, SEEK_SET);

	fData->Read(&entry.name, 8);
	fData->Read(entry.type);
	fData->Read(entry.key);

	return true;
}
