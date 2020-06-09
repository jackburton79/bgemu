#include "Archive.h"
#include "FileStream.h"
#include "MemoryStream.h"
#include "Resource.h"
#include "ResourceFactory.h"
#include "SupportDefs.h"
#include "IETypes.h"

#include <cstdio>


Resource::Resource(const res_ref &name, const uint16 &type)
	:
	Referenceable(0),
	fData(NULL),
	fKey(0),
	fType(type),
	fName(name)
{
}


Resource::~Resource()
{
	delete fData;
}


bool
Resource::Load(Archive *archive, uint32 key)
{
	fKey = key;

	delete fData;
	fData = archive->ReadResource(fName, key, fType);
	if (fData == NULL)
		return false;

	return true;
}


/* virtual */
Resource*
Resource::Clone()
{
	Resource* newResource = ResourceFactory().CreateResource(Name(), Type());
	newResource->fKey = fKey;
	newResource->fData = fData->Clone();
	newResource->Acquire();

	return newResource;
}


void
Resource::Dump()
{
	try {
		if (fData != NULL)
			fData->Dump();
	} catch (const char* string) {
		std::cerr << "Resource::Dump() caught exception: " << string << std::endl;
	} catch (...) {
		std::cerr << "Resource::Dump() caught exception." << std::endl;
	}
}


void
Resource::DumpToFile(const char *fileName)
{
	if (fData != NULL)
		fData->DumpToFile(fileName);
}


uint32
Resource::Key() const
{
	return fKey;
}


uint16
Resource::Type() const
{
	return fType;
}


const char*
Resource::Name() const
{
	return fName.CString();
}


bool
Resource::CheckSignature(const char *signature, bool dontWorry)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(0, array, 4) != 4)
		return false;

	if (strcmp(array, signature) != 0) {
		if (!dontWorry) {
			//printf("invalid signature %s (expected %s)\n",
				//	array, signature);
		}
		return false;
	}

	return true;
}


bool
Resource::CheckVersion(const char *version, bool dontWorry)
{
	char array[5];
	array[4] = '\0';

	if (fData->ReadAt(4, array, 4) != 4)
		return false;

	if (strcmp(array, version) != 0) {
		if (!dontWorry) {
			printf("invalid version %s (expected %s)\n",
				array, version);
		}
		return false;
	}

	return true;
}


bool
Resource::ReplaceData(Stream *stream)
{
	delete fData;
	fData = stream;

	return true;
}


void
Resource::DropData()
{
	ReplaceData(NULL);
}


bool
Resource::IsEncrypted()
{
	int32 pos = fData->Position();
	fData->Seek(0, SEEK_SET);

	bool encrypted = (fData->ReadByte() == 0xFF)
		&& (fData->ReadByte() == 0xFF);

	fData->Seek(pos, SEEK_SET);

	return encrypted;
}

