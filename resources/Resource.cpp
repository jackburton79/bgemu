#include "Resource.h"

#include "Archive.h"
#include "FileStream.h"
#include "Log.h"
#include "MemoryStream.h"
#include "SupportDefs.h"
#include "IETypes.h"

#include <cstdio>


Resource::Resource(const res_ref& name, const uint16& type)
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
Resource::Load(Archive* archive, uint32 key)
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
	Resource* newResource = Resource::Create(Name().c_str(), Type());
	newResource->fKey = fKey;
	newResource->fData = fData->Clone();
	newResource->Acquire();

	return newResource;
}


void
Resource::Dump()
{
	try {
		if (fData != NULL) {
			std::cout << Name() << ": size: " << fData->Size() << std::endl;
			fData->Dump();
		}
	} catch (std::exception& e) {
		std::cerr << RED("Resource::Dump() caught exception: ") << RED(e.what()) << std::endl;
	}
}


void
Resource::DumpToFile(const char* fileName)
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


std::string
Resource::Name() const
{
	return fName.CString();
}


bool
Resource::CheckSignature(const char* signature)
{
	char array[32];

	ssize_t len = ::strlen(signature);
	if (fData->ReadAt(0, array, len) != len)
		return false;

	array[len] = '\0';
	if (strncmp(array, signature, len) != 0) {
		std::cerr << "CheckSignature: expected " << signature;
		std::cerr << ", found " << array << " (not necessarily an error)" << std::endl;
		return false;
	}

	return true;
}


bool
Resource::CheckVersion(const char* version)
{
	char array[32];

	ssize_t len = ::strlen(version);
	if (fData->ReadAt(4, array, len) != len)
		return false;

	array[len] = '\0';
	if (strcmp(array, version) != 0) {
		std::cerr << "CheckVersion: expected " << version;
		std::cerr << ", found " << array << " (not necessarily an error)" << std::endl;
		return false;
	}

	return true;
}


bool
Resource::ReplaceData(Stream* stream)
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
	uint16 data = 0;
	fData->ReadAt(0, &data, sizeof(data));
	return data == 0xFFFF;
}


/* static */
Resource*
Resource::Create(const res_ref& name, const uint16& type)
{
	Resource* res = NULL;
	try {
		resource_creation_func creationFunction = get_resource_create(type);		
		res = creationFunction(name);
	} catch (std::exception& e) {
		std::cerr << RED("Resource::Create(): ") << RED(e.what()) << std::endl;
		res = NULL;
	} catch (...) {
		std::cerr << RED("Resource::Create(): exception thrown!") << std::endl;
		res = NULL;
	}

	return res;
}


/* static */
Resource*
Resource::Create(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive)
{
	Resource* resource = Resource::Create(name, type);
	if (resource != NULL) {
		if (!resource->Load(archive, key)) {
			delete resource;
			return NULL;
		}
	}

	return resource;
}

