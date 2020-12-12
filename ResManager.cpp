#include "2DAResource.h"
#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "BCSResource.h"
#include "CHUIResource.h"
#include "CreResource.h"
#include "Core.h"
#include "DLGResource.h"
#include "FileStream.h"
#include "GeneratedIDS.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "IETypes.h"
#include "KEYResource.h"
#include "MOSResource.h"
#include "MveResource.h"
#include "ResManager.h"
#include "Resource.h"
#include "SPLResource.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "Utils.h"
#include "VVCResource.h"
#include "WAVResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <limits.h>

#define USE_OVERRIDE 1

#define OVERRIDE_MASK	0x00
#define CACHE_MASK		0x01
#define CD_MASK			0xFC

#define LOC_ROOT		0x01
#define LOC_CD1			0x1 << 2
#define LOC_CD2			0x1 << 3
#define LOC_CD3			0x1 << 4
#define LOC_CD4			0x1 << 5
#define LOC_CD5			0x1 << 6

#define GET_CD(loc)			((loc) & CD_MASK)
#define IS_IN_CACHE(loc)	((loc) & CACHE_MASK)
#define IS_OVERRIDE(loc)	((loc) & OVERRIDE_MASK)

ResourceManager* gResManager = NULL;

static TLKResource* sDialogs;
static IDSResource* sAlignment;
static IDSResource* sGeneral;
static IDSResource* sAnimate;
static IDSResource* sAniSnd;
static IDSResource* sRaces;
static IDSResource* sGenders;
static IDSResource* sClasses;
static IDSResource* sSpecifics;
static IDSResource* sTriggers;
static IDSResource* sActions;
static IDSResource* sObjects;
static IDSResource* sEA;
static IDSResource* sGameTimes;
static IDSResource* sTimes;
static IDSResource* sTimeOfDays;

const char *kKeyResource = "Chitin.key";
const char *kDialogResource = "dialog.tlk";

const char* kComponentName = "ResourceManager: ";

ResourceManager::ResourceManager(const char* path)
	:
	fDebugLevel(0)
{
	// TODO: Move this elsewhere!
	IE::check_objects_size();

	std::cout << kComponentName << "Initialize(" << path << ")" << std::endl;
	fResourcesPath.SetTo(path);
	std::cout << "\t-> Set resources path to '" << fResourcesPath.String();
	std::cout << "'" << std::endl;

	KEYResource *key = GetKEY(kKeyResource);
	if (key == NULL)
		throw std::runtime_error("Cannot find key file");

	if (fDebugLevel > 0)
		key->Dump(); 

	const uint32 numBifs = key->CountFileEntries();
	for (uint32 b = 0; b < numBifs; b++) {
		if (KeyFileEntry* bif = key->GetFileEntryAt(b))
			fBifs.push_back(bif);
	}

	uint32 numResources = key->CountResourceEntries();
	for (uint32 c = 0; c < numResources; c++) {
		if (KeyResEntry *res = key->GetResEntryAt(c)) {
			ref_type refType;
			refType.name = res->name;
			refType.type = res->type;
			fResourceMap[refType] = res;
		} else {
			// TODO: There are some unnamed entries in BG2.
			// Check KeyResource.cpp
			//numResources--;
		}
	}

	if (fDebugLevel > 0) {
		std::cout << "\t-> Found " << numBifs << " BIF file entries ";
		std::cout << "and " << numResources << " resources." << std::endl;
	}
	key->Release();
}


ResourceManager::~ResourceManager()
{
	std::cout << kComponentName << "~ResourceManager()" << std::endl;

	gResManager->ReleaseResource(sTimeOfDays);
	gResManager->ReleaseResource(sTimes);
	gResManager->ReleaseResource(sGameTimes);
	gResManager->ReleaseResource(sEA);
	gResManager->ReleaseResource(sObjects);
	gResManager->ReleaseResource(sActions);
	gResManager->ReleaseResource(sTriggers);
	gResManager->ReleaseResource(sSpecifics);
	gResManager->ReleaseResource(sGenders);
	gResManager->ReleaseResource(sRaces);
	gResManager->ReleaseResource(sClasses);
	gResManager->ReleaseResource(sGeneral);
	gResManager->ReleaseResource(sAniSnd);
	gResManager->ReleaseResource(sAnimate);
	gResManager->ReleaseResource(sAlignment);
	gResManager->ReleaseResource(sDialogs);

	std::cout << kComponentName << "Deleting cached resources...";
	std::cout << std::endl;
	std::list<Resource*>::iterator it;
	for (it = fCachedResources.begin(); it != fCachedResources.end(); it++) {
		std::cout << "Deleting " << (*it)->Name();
		std::cout << "(" << strresource((*it)->Type()) << ")";
		std::cout << " (refcount = " << (*it)->RefCount() << ")..." << std::endl;
		delete *it;
	}

	std::cout << kComponentName << "Deleting resource_maps...";
	std::cout << std::endl;
	resource_map::iterator iter;
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		delete iter->second;
	}
	
	std::cout << kComponentName << "Deleting bifs maps...";
	std::cout << std::endl;
	bif_vector::iterator i;
	for (i = fBifs.begin(); i != fBifs.end(); i++) {
		delete *i;	
	}

	//TryEmptyResourceCache(true);

	archive_map::iterator aIter;
	for (aIter = fArchives.begin(); aIter != fArchives.end(); aIter++) {
		delete aIter->second;
	}

	std::cout << kComponentName << "Destroyed." << std::endl;
}


/* static */
bool
ResourceManager::Initialize(const char *path)
{
	try {
		if (gResManager == NULL)
			gResManager = new ResourceManager(path);
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}
	return true;
}


/* static */
void
ResourceManager::Destroy()
{
	delete gResManager;
}


void
ResourceManager::SetDebug(int level)
{
	fDebugLevel = level;
}


Resource*
ResourceManager::GetResource(const char* fullName)
{
	int type = res_string_to_type(fullName);
	std::string leaf = fullName;
	leaf = leaf.substr(0, leaf.find("."));
	res_ref name = leaf.c_str();

	return GetResource(name, type);
}


Resource*
ResourceManager::GetResource(const res_ref &name, uint16 type)
{
	if (!strcmp(name.name, ""))
		return NULL;

	KeyResEntry *entry = _GetKeyRes(name, type);
	if (entry == NULL) {
		std::cerr << kComponentName << "GetResource(";
		std::cerr << name.CString() << ", " << strresource(type);
		std::cerr << "): Resource does not exist!" << std::endl;
		return NULL;
	}

	/*std::cout << kComponentName << "GetResource(";
	std::cout << name.CString() << ", " << strresource(type);
	std::cout << ")" << std::endl;*/
	//std::cout << "\t-> Is it loaded ? ";
	//std::flush(std::cout);
	Resource *result = _FindResource(*entry);
	//if (result != NULL)
	//	std::cout << "YES";
	//else
	//	std::cout << "NO";
	//std::cout << std::endl;

#if USE_OVERRIDE
	if (result == NULL)
		result = _LoadResourceFromOverride(*entry);
#endif
	if (result == NULL)
		result = _LoadResource(*entry);

	if (result != NULL)
		result->Acquire();

	//std::cout << "\t" << "-> refcount " << result->RefCount() << std::endl;
	return result;
}


KEYResource*
ResourceManager::GetKEY(const char *name)
{
	KEYResource *key = NULL;
	Archive *archive = NULL;
	std::string path;
	key = new KEYResource("KEY");
	path = GetFullPath(name, LOC_ROOT);
	if (fDebugLevel > 0) {
		std::cout << "\t-> Loading KEY file '" << path << "'... ";
		std::flush(std::cout);
	}
	archive = Archive::Create(path.c_str());
	// TODO: Throw an useful exception instead
	if (archive == NULL)
		throw std::runtime_error("GetKey: cannot open key archive file!");
	if (key->Load(archive, 0) == false)
		throw std::runtime_error("GetKey: cannot load key file!");
	if (fDebugLevel > 0)
		std::cout << "OK!" << std::endl;

	delete archive;

	return key;
}


TLKResource*
ResourceManager::GetTLK(const char* name)
{
	Resource* tlk = NULL;
	Archive *archive = NULL;
	try {
		if (fDebugLevel > 0)
			std::cout << "\t-> Loading Dialogs file '" << name << "'... ";
		tlk = new TLKResource("TLK");
		std::string path = GetFullPath(name, LOC_ROOT);
		archive = Archive::Create(path.c_str());
		if (archive == NULL || tlk->Load(archive, 0) == false)
			return NULL;

		tlk->Acquire();
		if (fDebugLevel > 0)
			std::cout << "OK!" << std::endl;
	} catch (std::exception& e) {
		if (fDebugLevel > 0)
			std::cout << "FAILED!";
		std::cerr << e.what() << std::endl;
		if (tlk->Release())
			delete tlk;
		tlk = NULL;
	}

	delete archive;
	return dynamic_cast<TLKResource*>(tlk);
}


ARAResource*
ResourceManager::GetARA(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_ARA);
	return static_cast<ARAResource*>(resource);
}


BAMResource*
ResourceManager::GetBAM(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_BAM);
	return static_cast<BAMResource*>(resource);
}


BMPResource*
ResourceManager::GetBMP(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_BMP);
	return static_cast<BMPResource*>(resource);
}


BCSResource*
ResourceManager::GetBCS(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_BCS);
	return static_cast<BCSResource*>(resource);
}


CHUIResource*
ResourceManager::GetCHUI(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_CHU);
	return static_cast<CHUIResource*>(resource);
}


CREResource*
ResourceManager::GetCRE(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_CRE);
	return static_cast<CREResource*>(resource);
}


DLGResource*
ResourceManager::GetDLG(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_DLG);
	return static_cast<DLGResource*>(resource);
}


IDSResource*
ResourceManager::GetIDS(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_IDS);
	return static_cast<IDSResource*>(resource);
}


ITMResource*
ResourceManager::GetITM(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_ITM);
	return static_cast<ITMResource*>(resource);
}


MOSResource*
ResourceManager::GetMOS(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_MOS);
	return static_cast<MOSResource*>(resource);
}


MVEResource*
ResourceManager::GetMVE(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_MVE);
	return static_cast<MVEResource*>(resource);
}


TISResource*
ResourceManager::GetTIS(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_TIS);
	return static_cast<TISResource*>(resource);
}


TWODAResource*
ResourceManager::Get2DA(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_2DA);
	return static_cast<TWODAResource*>(resource);
}


SPLResource*
ResourceManager::GetSPL(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_SPL);
	return static_cast<SPLResource*>(resource);
}


WEDResource*
ResourceManager::GetWED(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_WED);
	return static_cast<WEDResource*>(resource);
}


WMAPResource*
ResourceManager::GetWMAP(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_WMP);
	return static_cast<WMAPResource*>(resource);
}


VVCResource*
ResourceManager::GetVVC(const res_ref& name)
{
	Resource* resource = GetResource(name, RES_VVC);
	return static_cast<VVCResource*>(resource);
}


void
ResourceManager::GetCachedResourcesList(StringList& list)
{
	std::list<Resource*>::iterator iter;
	for (iter = fCachedResources.begin(); iter != fCachedResources.end(); iter++) {
		std::string resource = (*iter)->Name();
		resource.append("(");
		resource.append(strresource((*iter)->Type()));
		resource.append(")");
		list.push_back(resource);
	}
}


void
ResourceManager::ReleaseResource(Resource* resource)
{
	if (resource != NULL) {
		/*const int32 refCount = resource->RefCount();
		std::cout << kComponentName << "ReleaseResource(";
		std::cout << resource->Name() << ", " << strresource(resource->Type());
		std::cout << ")";
		std::cout << ": refcount was " << refCount;*/
		if (resource->Release()) {
			delete resource;
			//std::cout << " and is now 0. Resource deleted";
		} /*else
			std::cout << " and is now " << resource->RefCount();
		std::cout << "." << std::endl;*/
	}
}


int32
ResourceManager::GetResourceList(std::vector<std::string>& strings,
		const char* query, uint16 type) const
{
	//std::cout << "GetResourceList(" << query << "):" << std::endl;
	resource_map::const_iterator iter;
	const int32 queryLen = strlen(query);
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		const ref_type resType = (*iter).first;
		std::string newString(resType.name.CString());
		if (resType.type == type
				&& !strncmp(newString.c_str(), query, queryLen)) {
			strings.push_back(newString);
		}
	}

	return strings.size();
}


std::string
ResourceManager::GetFullPath(std::string name, uint16 location)
{
	//std::cout << "ResourceManager::GetFullPath(" << name << ", 0x";
	//std::cout << std::hex << location << ")" << std::endl;

	Path pathName(fResourcesPath);
	if (pathName.InitCheck() != 0) {
		std::cerr << "Invalid path" << std::endl;
		throw std::runtime_error("Invalid path");
	}
	
	// TODO: Introduce the concept of a "current cd"
	// although since the game is fully installed it doesn't
	// really matter
	std::string locationString = "( In ";	
	uint32 cd = GET_CD(location);
	if ((location & LOC_ROOT) == 0) {
		//if (IS_OVERRIDE(location))
		//	printf("\tshould check in override\n");
		// TODO: this represents the LIST of cd where
		// we can find the resource.
		// some resources exist on many cds.
		if (cd & LOC_CD1) {
			pathName.Append("CD1/");
			locationString.append("CD 1");
		} else if (cd & LOC_CD2) {
			pathName.Append("CD2/");
			locationString.append("CD 2");
		} else if (cd & LOC_CD3) {
			pathName.Append("CD3/");
			locationString.append("CD 3");
		} else if (cd & LOC_CD4) {
			pathName.Append("CD4/");
			locationString.append("CD 4");
		} else if (cd & LOC_CD5) {
			pathName.Append("CD5/");
			locationString.append("CD 5");
		}
	} else
		locationString.append("ROOT");
	
	locationString.append(" )");
	
	//std::cout << locationString;

	//printf("CD: 0x%x ", GET_CD(location));
	//std::cout << std::endl;

	pathName.Append(name.c_str(), false);

	return pathName.String();
}


Resource*
ResourceManager::_LoadResource(KeyResEntry &entry)
{
	const int bifIndex = RES_BIF_INDEX(entry.key);
	const uint16& location = fBifs[bifIndex]->location;
	const char* archiveName = fBifs[bifIndex]->name;

	if (fDebugLevel > 0) {
		std::cout << kComponentName << "LoadResource(";
		std::cout << entry.name.CString() << ", " << strresource(entry.type);
		std::cout << ")" << std::endl;
	}
	Archive *archive = fArchives[archiveName];
	if (archive == NULL) {
		std::string fullPath = GetFullPath(archiveName, location);
		if (fDebugLevel > 0) {
			std::cout << "\t-> Loading archive '" << fullPath << "'... ";
			std::flush(std::cout);
		}
		archive = Archive::Create(fullPath.c_str());
		if (archive == NULL) {
			if (fDebugLevel > 0)
				std::cout << "FAILED!" << std::endl;
			return NULL;
		}
		if (fDebugLevel > 0)
			std::cout << "OK!" << std::endl;
		fArchives[archiveName] = archive;
	}

	Resource *resource = Resource::Create(entry.name, entry.type,
										entry.key, archive);
	if (resource == NULL) {
		if (fDebugLevel > 0)
			std::cout << kComponentName << "FAILED Loading resource!" << std::endl;
		delete resource;
		return NULL;
	}

	resource->Acquire();
	fCachedResources.push_back(resource);
	if (fDebugLevel > 0) {
		std::cout << "\t-> Resource " << entry.name.CString();
		std::cout << " (" << strresource(entry.type) << ") ";
		std::cout << "loaded correctly!" << std::endl;
	}
	return resource;
}


Resource*
ResourceManager::_LoadResourceFromOverride(KeyResEntry& entry)
{
	// TODO: Try the other override directories (dialogs, characters, etc.... override)

	Resource *resource = NULL;
	if (entry.type == RES_BCS)
		resource = _LoadResourceFromOverride(entry, "scripts");
	if (resource == NULL)
		resource = _LoadResourceFromOverride(entry, "override");
	return resource;
}


Resource*
ResourceManager::_LoadResourceFromOverride(KeyResEntry& entry,
		const char* overridePath)
{
	std::string fullPath = GetFullPath(overridePath, LOC_ROOT);

	Archive* dirArchive = Archive::Create(fullPath.c_str());

	//std::cout << "Archive created" << std::endl;
	// TODO: Merge the code with the rest ?
	if (dirArchive == NULL)
		return NULL;

	Resource *resource = Resource::Create(entry.name, entry.type,
										entry.key, dirArchive);
	if (resource == NULL) {
		delete dirArchive;
		delete resource;
		return NULL;
	}

	resource->Acquire();
	fCachedResources.push_back(resource);

	if (fDebugLevel > 0) {
		std::cout << "Resource " << entry.name << "(";
		std::cout << strresource(entry.type) << ")";
		std::cout << "loaded correctly from override!" << std::endl;
	}
	delete dirArchive;
	return resource;
}


void
ResourceManager::PrintResources(int32 type)
{
	std::cout << kComponentName;
	std::cout << "Listing " << fResourceMap.size();
	std::cout << " entries..." << std::endl;
	resource_map::iterator iter;
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		KeyResEntry *res = iter->second;
		if (res == NULL) {
			std::cerr << "KeyResEntry is NULL. SHOULD NOT HAPPEN! ";
			std::cerr << iter->first.name << " (";
			std::cerr << iter->first.type << " )" << std::endl;
			abort();
			continue;
		}
		if (type == -1 || type == res->type) {
			std::cout << res->name << " " << strresource(res->type);
			std::cout << ", " << fBifs[RES_BIF_INDEX(res->key)]->name;
			std::cout << ", index " << RES_BIF_FILE_INDEX(res->key);
			std::cout << std::endl;
		}
	}
}


void
ResourceManager::PrintBIFs()
{
	bif_vector::iterator iter;
	for (iter = fBifs.begin(); iter != fBifs.end(); iter++) {
		KeyFileEntry *entry = *iter;
		if (fDebugLevel > 0) {
			std::cout << iter - fBifs.begin() << "\t" << entry->name;
			std::cout << "\t" << std::hex << entry->location << endl;
		}
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


Resource*
ResourceManager::_FindResource(KeyResEntry &entry)
{
	std::list<Resource*>::iterator iter;
	for (iter = fCachedResources.begin(); iter != fCachedResources.end(); iter++) {
		if ((*iter)->Key() == entry.key)
			return *iter;
	}
	return NULL;
}


KeyResEntry*
ResourceManager::_GetKeyRes(const res_ref &name, uint16 type) const
{
	ref_type nameType = { name, type };
	resource_map::const_iterator iter = fResourceMap.find(nameType);
	if (iter == fResourceMap.end())
		return NULL;

	return iter->second;
}


void
ResourceManager::TryEmptyResourceCache(bool force)
{
	std::list<Resource*>::iterator it = fCachedResources.begin();
	while (it != fCachedResources.end()) {
		//std::cout << kComponentName;
		//std::cout << (*it)->Name() << "(" << strresource((*it)->Type()) << "): ";
		//std::cout << "refcount is " << (*it)->RefCount();
		if (force || (*it)->RefCount() == 1) {
		//	std::cout << ": Deleting...";
			std::flush(std::cout);
			delete *it;
			it = fCachedResources.erase(it);
		} else
			it++;
		//std::cout << std::endl;
	}
}


// IDTable
TLKResource*
Dialogs()
{
	if (sDialogs == NULL) {
		sDialogs = gResManager->GetTLK(kDialogResource);
		if (sDialogs == NULL)
			throw std::runtime_error("Cannot load dialog resource!");
	}
	return sDialogs;
}


/* static */
std::string
IDTable::GetDialog(uint32 i)
{
	std::string text;
	TLKEntry* entry = GetTLK(i);
	if (entry != NULL) {
		text = entry->text;
		delete entry;
	}
	return text;
}


TLKEntry*
IDTable::GetTLK(uint32 i)
{
	if (sDialogs == NULL)
		sDialogs = gResManager->GetTLK(kDialogResource);
	if (sDialogs != NULL)
		return sDialogs->EntryAt(i);
	assert(sDialogs != NULL);	
	return NULL;
}


std::string
IDTable::AlignmentAt(uint32 i)
{
	if (sAlignment == NULL)
		sAlignment = gResManager->GetIDS("ALIGNMENT");

	return sAlignment->StringForID(i);
}


std::string
IDTable::GeneralAt(uint32 i)
{
	if (sGeneral == NULL)
		sGeneral = gResManager->GetIDS("GENERAL");

	return sGeneral->StringForID(i);
}


std::string
IDTable::AnimationAt(uint32 i)
{
	if (sAnimate == NULL)
		sAnimate = gResManager->GetIDS("ANIMATE");
	return sAnimate->StringForID(i);
}


std::string
IDTable::AniSndAt(uint32 i)
{
	if (sAniSnd == NULL) {
		sAniSnd = gResManager->GetIDS("ANISND");
		if (sAniSnd == NULL) {
			// No AniSnd.ids file, let's use our own.
			sAniSnd = GeneratedIDS::CreateIDSResource("ANISND");
			// Acquire a reference in this case, since Resources starts
			// with a refcount of 0
			sAniSnd->Acquire();
		}
	}
	return sAniSnd->StringForID(i);
}


std::string
IDTable::RaceAt(uint32 i)
{
	if (sRaces == NULL)
		sRaces = gResManager->GetIDS("RACE");

	return sRaces->StringForID(i);
}


std::string
IDTable::GenderAt(uint32 i)
{
	if (sGenders == NULL)
		sGenders = gResManager->GetIDS("GENDER");
	return sGenders->StringForID(i);
}


uint32
IDTable::GenderID(std::string string)
{
	if (sGenders == NULL)
		sGenders = gResManager->GetIDS("GENDER");
	return sGenders->IDForString(string);
}


std::string
IDTable::ClassAt(uint32 i)
{
	if (sClasses == NULL)
		sClasses = gResManager->GetIDS("CLASS");
	return sClasses->StringForID(i);
}


std::string
IDTable::SpecificAt(uint32 i)
{
	if (sSpecifics == NULL)
		sSpecifics = gResManager->GetIDS("SPECIFIC");

	return sSpecifics->StringForID(i);
}


std::string
IDTable::TriggerName(uint32 i)
{
	if (sTriggers == NULL)
		sTriggers = gResManager->GetIDS("TRIGGER");

	return sTriggers->StringForID(i);
}


uint32
IDTable::TriggerID(std::string name)
{
	if (sTriggers == NULL)
		sTriggers = gResManager->GetIDS("TRIGGER");
	return sTriggers->IDForString(name);	
}


std::string
IDTable::ActionAt(uint32 i)
{
	if (sActions == NULL)
		sActions = gResManager->GetIDS("ACTION");
	return sActions->StringForID(i);
}


uint32
IDTable::ObjectID(std::string string)
{
	if (sObjects == NULL)
		sObjects = gResManager->GetIDS("OBJECT");
	return sObjects->IDForString(string);
}


std::string
IDTable::ObjectAt(uint32 i)
{
	if (sObjects == NULL)
		sObjects = gResManager->GetIDS("OBJECT");
	return sObjects->StringForID(i);
}


std::string
IDTable::EnemyAllyAt(uint32 i)
{
	if (sEA == NULL)
		sEA = gResManager->GetIDS("EA");
	return sEA->StringForID(i);
}


uint32
IDTable::EnemyAllyValue(std::string string)
{
	if (sEA == NULL)
		sEA = gResManager->GetIDS("EA");
	return sEA->IDForString(string);
}


std::string
IDTable::GameTimeAt(uint32 i)
{
	if (sGameTimes == NULL)
		sGameTimes = gResManager->GetIDS("GTIMES");
	return sGameTimes->StringForID(i);
}


std::string
IDTable::TimeAt(uint32 i)
{
	if (sTimes == NULL)
		sTimes = gResManager->GetIDS("TIME");
	return sTimes->StringForID(i);
}


std::string
IDTable::TimesOfDayAt(uint32 i)
{
	if (sTimeOfDays == NULL)
		sTimeOfDays = gResManager->GetIDS("TIMEODAY");
	return sTimeOfDays->StringForID(i);
}

