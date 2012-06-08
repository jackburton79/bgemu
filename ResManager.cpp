#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BCSResource.h"
#include "BmpResource.h"
#include "CreResource.h"
#include "IDSResource.h"
#include "KEYResource.h"
#include "MveResource.h"
#include "ResManager.h"
#include "Resource.h"
#include "FileStream.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "IETypes.h"
#include "Utils.h"
#include "WedResource.h"

#include <assert.h>
#include <iostream>
#include <limits.h>


#define OVERRIDE_MASK	0x00
#define CACHE_MASK		0x01
#define CD_MASK			0xFC

#define LOC_ROOT		0x01
#define LOC_CD1			0x04
#define LOC_CD2			0x08
#define LOC_CD3			0x10
#define LOC_CD4			0x20
#define LOC_CD5			0x40

#define GET_CD(loc)			((loc) & CD_MASK)
#define IS_IN_CACHE(loc)	((loc) & CACHE_MASK)
#define IS_OVERRIDE(loc)	((loc) & OVERRIDE_MASK)


using namespace std;

ResourceManager *gResManager = NULL;
static ResourceManager sManager;

const char *kKeyResource = "Chitin.key";
const char *kDialogResource = "dialog.tlk";


ResourceManager::ResourceManager()
{
	// TODO: Move this elsewhere!
	IE::check_objects_size();
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


bool
ResourceManager::Initialize(const char *path)
{
	fResourcesPath.SetTo(path);

	KEYResource *key = GetKEY(kKeyResource);
	if (key == NULL)
		return false;

	const uint32 numBifs = key->CountFileEntries();
	for (uint32 b = 0; b < numBifs; b++) {
		KeyFileEntry *bif = new KeyFileEntry;
		if (key->GetFileEntryAt(b, *bif))
			fBifs.push_back(bif);
		else
			delete bif;
	}

	bool resourcesOk = true;
	try {
		const uint32 numResources = key->CountResourceEntries();
		for (uint32 c = 0; c < numResources; c++) {
			KeyResEntry *res = new KeyResEntry;
			if (key->GetResEntryAt(c, *res)) {
				ref_type refType;
				refType.name = res->name;
				if (!strncmp(res->name, "", 8)) {
					// TODO: looks like we get some unnamed resources
					// and this causes all kinds of problems. Investigate
					delete res;
					continue;
				}
				refType.type = res->type;
				fResourceMap[refType] = res;
			} else
				delete res;
		}
	} catch (...) {
		printf("Error!!!\n");
		resourcesOk = false;
	}

	delete key;

	return resourcesOk;
}


Resource *
ResourceManager::_GetResource(const res_ref &name, uint16 type)
{
	if (!strcmp(name.name, ""))
		return NULL;

	KeyResEntry *entry = _GetKeyRes(name, type);
	if (entry == NULL) {
		printf("ResourceManager::GetResource(%s, %s): Resource does not exist!\n",
				(const char *)name, strresource(type));
		return NULL;
	}

	Resource *result = _FindResource(*entry);
	if (result == NULL) {
		//printf("\tnot found in cache. Loading...\n");
		result = _LoadResource(*entry);
	}

	if (result != NULL)
		result->_Acquire();

	return result;
}


KEYResource *
ResourceManager::GetKEY(const char *name)
{
	KEYResource *key = NULL;
	try {
		key = new KEYResource("KEY");
		std::string path = GetFullPath(name, LOC_ROOT);
		Archive *archive = Archive::Create(path.c_str());
		if (!key->Load(archive, 0)) {
			delete archive;
			delete key;
			return NULL;
		}
		//key->_Acquire();
		delete archive;
	} catch (...) {
		return NULL;
	}
	return key;
}


TLKResource *
ResourceManager::GetTLK(const char *name)
{
	TLKResource *tlk = NULL;
	try {
		tlk = new TLKResource("TLK");
		std::string path = GetFullPath(name, LOC_ROOT);
		Archive *archive = Archive::Create(path.c_str());
		tlk->Load(archive, 0);
		tlk->_Acquire();
		delete archive;
	} catch (...) {
		return NULL;
	}
	return tlk;
}


ARAResource *
ResourceManager::GetARA(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_ARA);
	//assert(dynamic_cast<ARAResource *>(resource));

	return static_cast<ARAResource *>(resource);
}


BAMResource *
ResourceManager::GetBAM(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_BAM);
	assert(dynamic_cast<BAMResource *>(resource));
	
	return static_cast<BAMResource *>(resource);
}


BMPResource *
ResourceManager::GetBMP(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_BMP);
	//assert(dynamic_cast<BMPResource *>(resource));

	return static_cast<BMPResource *>(resource);
}


BCSResource *
ResourceManager::GetBCS(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_BCS);
	//assert(dynamic_cast<BCSResource *>(resource));

	return static_cast<BCSResource *>(resource);
}


CREResource *
ResourceManager::GetCRE(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_CRE);
	//assert(dynamic_cast<CREResource *>(resource));

	return static_cast<CREResource *>(resource);
}


IDSResource *
ResourceManager::GetIDS(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_IDS);
	//assert(dynamic_cast<IDSResource *>(resource));

	return static_cast<IDSResource *>(resource);
}


MVEResource *
ResourceManager::GetMVE(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_MVE);
	//assert(dynamic_cast<ARAResource *>(resource));

	return static_cast<MVEResource *>(resource);
}


TISResource *
ResourceManager::GetTIS(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_TIS);
	//assert(dynamic_cast<TISResource *>(resource));
	
	return static_cast<TISResource *>(resource);
}


WEDResource *
ResourceManager::GetWED(const res_ref &name)
{
	Resource *resource = _GetResource(name, RES_WED);
	//assert(dynamic_cast<WEDResource *>(resource));

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

	//printf("(0x%x)", GET_CD(location));

	if ((location & LOC_ROOT) == 0) {
		if (IS_OVERRIDE(location))
			printf("\tshould check in override\n");
		// TODO: this represents the LIST of cd where we can find the resource.
		// some resources exist on many cds.
		uint32 cd = GET_CD(location);
		if (cd & LOC_CD1)
			pathName.Append("CD1/");
		else if (cd & LOC_CD2)
			pathName.Append("CD2/");
		else if (cd & LOC_CD3)
			pathName.Append("CD3/");
		else if (cd & LOC_CD4)
			pathName.Append("CD4/");
		else if (cd & LOC_CD5)
			pathName.Append("CD5/");
	}

	printf("CD: 0x%x\n", GET_CD(location));
	pathName.Append(name.c_str());

	return pathName.Path();
}


Resource *
ResourceManager::_LoadResource(KeyResEntry &entry)
{
	printf("ResourceManager::LoadResource(%s, %s)...",
		(const char *)entry.name, strresource(entry.type));

	// TODO: Try the override directory

	const int bifIndex = RES_BIF_INDEX(entry.key);
	const uint16 location = fBifs[bifIndex]->location;
	const char *archiveName = fBifs[bifIndex]->name;

	printf("(in %s (0x%x))\n", archiveName, location);

	Archive *archive = fArchives[archiveName];
	if (archive == NULL) {
		printf("\tArchive wasn't opened. Opening...");
		std::string fullPath = GetFullPath(archiveName, location);
		archive = Archive::Create(fullPath.c_str());
		if (archive == NULL) {
			printf("FAILED!\n");
			return NULL;
		}

        fArchives[archiveName] = archive;
	}

	Resource *resource = Resource::Create(entry.name, entry.type);
	if (resource == NULL || !resource->Load(archive, entry.key)) {
		printf("FAILED Loading resource!\n");
		delete resource;
		return NULL;
	}

	resource->_Acquire();
	fCachedResources.push_back(resource);

	printf("Resource %s (%s) loaded correctly!\n",
			(const char *)entry.name, strresource(entry.type));
	return resource;
}


void
ResourceManager::PrintResources()
{
	printf("Listing %d entries...\n", fResourceMap.size());
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


/* static */
std::string
ResourceManager::HeightMapName(const char *name)
{
	std::string hmName = name;
	hmName.append("HT");
	return hmName;
}


/* static */
std::string
ResourceManager::LightMapName(const char *name)
{
	std::string lmName = name;
	lmName.append("LM");
	return lmName;
}


/* static */
std::string
ResourceManager::SearchMapName(const char *name)
{
	std::string srName = name;
	srName.append("SR");
	return srName;
}


Resource *
ResourceManager::_FindResource(KeyResEntry &entry)
{
	std::vector<Resource *>::iterator iter;
	for (iter = fCachedResources.begin(); iter != fCachedResources.end(); iter++) {
		if ((*iter)->Key() == entry.key)
			return *iter;
	}
	return NULL;
}


KeyResEntry *
ResourceManager::_GetKeyRes(const res_ref &name, uint16 type)
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

