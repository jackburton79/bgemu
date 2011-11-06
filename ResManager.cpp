#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "BGCreature.h"
#include "ResManager.h"
#include "Resource.h"
#include "FileStream.h"
#include "TisResource.h"
#include "Types.h"
#include "Utils.h"
#include "WedResource.h"

#include <assert.h>
#include <iostream>
#include <limits.h>


#define OVERRIDE_MASK	0x00
#define CACHE_MASK		0x01
#define CD_MASK			0x70

#define LOC_ROOT		0x01
#define LOC_CD2			0x00
#define LOC_CD3			0x10
#define LOC_CD4			0x20
#define LOC_CD5			0x40

#define GET_CD(loc)			((loc) & CD_MASK)
#define IS_IN_CACHE(loc)	((loc) & CACHE_MASK)
#define IS_OVERRIDE(loc)	((loc) & OVERRIDE_MASK)


using namespace std;

ResourceManager *gResManager = NULL;
static ResourceManager sManager;


ResourceManager::ResourceManager()
{
	// TODO: Move this elsewhere!
	check_objects_size();
	gResManager = &sManager;
}


ResourceManager::~ResourceManager()
{
	resource_map::iterator iter;
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		delete iter->second;
	}
	
	bif_vector::iterator i;
	for (i = fBifs.begin(); i != fBifs.end(); i++) {
		delete *i;	
	}

	/*std::vector<Resource *>::iterator it;
	for (it = fCachedResources.begin(); it != fCachedResources.end(); it++)
		delete *it;
*/
	_TryEmptyResourceCache();

	archive_map::iterator aIter;
	for (aIter = fArchives.begin(); aIter != fArchives.end(); aIter++) {
		delete aIter->second;
	}
}


void
ResourceManager::SetResourcesPath(const char *path)
{
	fResourcesPath.SetTo(path);
}


void
ResourceManager::ParseKeyFile(const char *fileName)
{
	TPath filePath(fResourcesPath.Path(), fileName);
	TFileStream file(filePath.Path(),
			TFileStream::READ_ONLY,
			TFileStream::CASE_INSENSITIVE);

	char array[5];
	array[4] = '\0';
	
	file.Read(array, 4);
	if (strcmp(array, KEY_SIGNATURE)) {
		printf("bad signature\n");
		return;
	}
	
	file.Read(array, 4); // version
	if (strcmp(array, KEY_VERSION)) {
		printf("bad version\n");
		return;
	}
	
	int32 numBifs;
	file >> numBifs;
	
	int32 numResources;
	file >> numResources;
	
	int32 bifOffset;
	file >> bifOffset;
	
	int32 resOffset;
	file >> resOffset;
	
	// Retrieve BIF entries
	file.Seek(bifOffset, SEEK_SET);
	for (int i = 0; i < numBifs; i++) {
		KeyFileEntry *bif = new KeyFileEntry;
		
		int32 offset;
		int16 nameLen;
		
		file >> bif->length;		
		file >> offset;
		file >> nameLen;
		file >> bif->location;
		
		char *name = new char[nameLen];
		file.ReadAt(offset, name, nameLen);
		
		path_dos_to_unix(name);
		
		bif->name = name;
		delete[] name;
		
		fBifs.push_back(bif);
		
	}
	
	// Retrieve resource entries	
	file.Seek(resOffset, SEEK_SET);	
	for (int32 c = 0; c < numResources; c++) {
		KeyResEntry *res = new KeyResEntry;
		
		file.Read((void*)&res->name, 8);
		file >> res->type >> res->key;

		ref_type refType = { res->name, res->type };
		fResourceMap[refType] = res;
	}
/*
	TPath path = fResourcesPath;
	path.Append("override/");
	TArchive *archive = TArchive::Create(path.Path());
	if (archive != NULL)
		archive->EnumEntries();*/
}


Resource *
ResourceManager::GetResource(const res_ref &name, uint16 type)
{
	KeyResEntry *entry = GetKeyRes(name, type);
	if (entry == NULL) {
		printf("ResourceManager::GetResource(%s, %s): Resource does not exist!\n",
				(const char *)name, strresource(type));
		return NULL;
	}

	Resource *result = FindResource(*entry);
	if (result == NULL) {
		//printf("\tnot found in cache. Loading...\n");
		result = LoadResource(*entry);
	}

	if (result != NULL)
		result->_Acquire();

	return result;
}


AREAResource *
ResourceManager::GetAREA(const res_ref &name)
{
	Resource *resource = GetResource(name, RES_AREA);
	assert(dynamic_cast<AREAResource *>(resource));

	return static_cast<AREAResource *>(resource);
}


BAMResource *
ResourceManager::GetBAM(const res_ref &name)
{
	Resource *resource = GetResource(name, RES_BAM);
	assert(dynamic_cast<BAMResource *>(resource));
	
	return static_cast<BAMResource *>(resource);
}


BMPResource *
ResourceManager::GetBMP(const res_ref &name)
{
	Resource *resource = GetResource(name, RES_BMP);
	assert(dynamic_cast<BMPResource *>(resource));

	return static_cast<BMPResource *>(resource);
}


TISResource *
ResourceManager::GetTIS(const res_ref &name)
{
	Resource *resource = GetResource(name, RES_TIS);
	assert(dynamic_cast<TISResource *>(resource));
	
	return static_cast<TISResource *>(resource);
}


WEDResource *
ResourceManager::GetWED(const res_ref &name)
{
	Resource *resource = GetResource(name, RES_WED);
	assert(dynamic_cast<WEDResource *>(resource));

	return static_cast<WEDResource *>(resource);
}


void
ResourceManager::ReleaseResource(Resource *resource)
{
	if (resource != NULL)
		resource->_Release();
}


std::string
ResourceManager::GetFullPath(std::string name, uint16 location)
{
	TPath pathName(fResourcesPath);

	if ((location & LOC_ROOT) == 0) {
		if (IS_OVERRIDE(location))
			printf("\tshould check in override\n");
		switch (GET_CD(location)) {
			case LOC_CD2:
				pathName.Append("CD2/");
				break;
			case LOC_CD3:
				pathName.Append("CD3/");
				break;
			case LOC_CD4:
				pathName.Append("CD4/");
				break;
			case LOC_CD5:
				pathName.Append("CD5/");
				break;
			default:
				break;
		}
	}

	pathName.Append(name.c_str());

	return pathName.Path();
}


Resource *
ResourceManager::LoadResource(KeyResEntry &entry)
{
	printf("ResourceManager::LoadResource(%s, %s)...",
		(const char *)entry.name, strresource(entry.type));

	const int bifIndex = RES_BIF_INDEX(entry.key);
	const uint16 location = fBifs[bifIndex]->location;
	const char *archiveName = fBifs[bifIndex]->name.data();

	printf("LOCATED!\n\t(in %s (0x%x))\n", archiveName, location);

	TArchive *archive = fArchives[archiveName];
	if (archive == NULL) {
		printf("\tArchive wasn't opened. Opening...\n");
		std::string fullPath = GetFullPath(archiveName, location);
		archive = TArchive::Create(fullPath.c_str());
		if (archive == NULL) {
			printf("FAILED!\n");
			return NULL;
		}

        fArchives[archiveName] = archive;
	}

	printf("\tloading resource...\n");
	Resource *resource = Resource::Create(entry.name, entry.type);
	if (resource == NULL || !resource->Load(archive, entry.key)) {
		printf("FAILED!\n");
		delete resource;
		return NULL;
	}

	resource->_Acquire();
	fCachedResources.push_back(resource);
		
	return resource;
}


void
ResourceManager::PrintResources()
{
	resource_map::iterator iter;
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		KeyResEntry *res = iter->second;
		cout << res->name << " " << strresource(res->type);
		cout << ", " << fBifs[RES_BIF_INDEX(res->key)]->name;
		cout << ", index " << RES_BIF_FILE_INDEX(res->key);
		cout << endl;	
	}
}


void
ResourceManager::PrintBIFs()
{
	bif_vector::iterator iter;
	for (iter = fBifs.begin(); iter != fBifs.end(); iter++) {
		KeyFileEntry *entry = *iter;
		cout << iter - fBifs.begin() << "\t" << entry->name;
		cout << "\t" << hex << entry->location << endl;
	}
}


std::string
ResourceManager::HeightMapName(const char *name)
{
	std::string hmName = name;
	hmName.append("HT");
	return hmName;
}


std::string
ResourceManager::LightMapName(const char *name)
{
	std::string lmName = name;
	lmName.append("LM");
	return lmName;
}


std::string
ResourceManager::SearchMapName(const char *name)
{
	std::string srName = name;
	srName.append("SR");
	return srName;
}


Resource *
ResourceManager::FindResource(KeyResEntry &entry)
{
	std::vector<Resource *>::iterator iter;
	for (iter = fCachedResources.begin(); iter != fCachedResources.end(); iter++) {
		if ((*iter)->Key() == entry.key)
			return *iter;
	}
	return NULL;
}


KeyResEntry *
ResourceManager::GetKeyRes(const res_ref &name, uint16 type)
{
	ref_type nameType = {name, type};
	return fResourceMap[nameType];
}


void
ResourceManager::_TryEmptyResourceCache()
{
	std::vector<Resource *>::iterator it;
	it = fCachedResources.begin();
	while (fCachedResources.size() > 0) {
		if ((*it)->_Release())
			delete *it;
		it = fCachedResources.erase(it);
	}
}
