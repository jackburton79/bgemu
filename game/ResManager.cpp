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
#include "GeneratedIDS.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "IETypes.h"
#include "KeyDatabase.h"
#include "KEYResource.h"
#include "Log.h"
#include "MOSResource.h"
#include "MveResource.h"
#include "ResManager.h"
#include "Resource.h"
#include "SPLResource.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "VVCResource.h"
#include "WedResource.h"
#include "WMAPResource.h"

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
	fKeyDB(NULL),
	fDebugLevel(0)
{
	// TODO: Move this elsewhere!
	IE::check_objects_size();

	std::cout << "ResourceManager: set resources path to '";
	fResourcesPath.SetTo(path);
	std::cout << fResourcesPath.String();
	std::cout << "' (from '" << path;
	std::cout << "')" << std::endl;

	fKeyDB = new KeyDatabase();

	std::string keyFilePath = GetFullPath(kKeyResource, LOC_ROOT);
	if (!fKeyDB->Load(keyFilePath.c_str())) {
		throw std::runtime_error("Cannot find key file");
	}
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


	std::cout << kComponentName << "Cached resources...";
	std::cout << std::endl;
	for (auto resource: fCachedResources) {
			//if (fDebugLevel > 0) {
		std::cout << "resource: " << resource.second->Name();
		std::cout << "(" << strresource(resource.second->Type()) << ")";
		std::cout << " (refcount = " << resource.second->RefCount() << ")..." << std::endl;
			//}
	}
	std::cout << kComponentName << "Deleting cached resources...";
	std::cout << std::endl;
	for (auto resource: fCachedResources) {
		//if (fDebugLevel > 0) {
			std::cout << "Deleting " << resource.second->Name();
			std::cout << "(" << strresource(resource.second->Type()) << ")";
			std::cout << " (refcount = " << resource.second->RefCount() << ")..." << std::endl;
		//}
		if (resource.second->Release())
			delete resource.second;
	}

	delete fKeyDB;
	//TryEmptyResourceCache(true);

	for (auto archive: fArchives) {
		delete archive.second;
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
		std::cerr << RED(e.what()) << std::endl;
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


bool
ResourceManager::ResourceExists(const res_ref& ref, uint16 type) const
{
	return fKeyDB->Find({ ref, type }) != NULL;
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
	if (!strcmp(name.CString(), "")) {
		std::cerr << Log::Yellow << kComponentName << "GetResource() called with empty name!" << std::endl;
		std::cerr << Log::Normal;
		return NULL;
	}

	const ref_type id = {name, type};
	const KeyResEntry *entry = fKeyDB->Find(id);
	if (entry == NULL) {
		std::cerr << RED(kComponentName) << RED("GetResource(");
		std::cerr << RED(name.CString()) << RED(", ") << RED(strresource(type));
		std::cerr << RED("): Resource does not exist!") << std::endl;
		return NULL;
	}

	Resource *result = _FindResource(*entry);
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
			std::cout << GREEN("OK!") << std::endl;
	} catch (std::exception& e) {
		if (fDebugLevel > 0)
			std::cout << RED("FAILED!");
		std::cerr << RED(e.what()) << std::endl;
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
	for (auto resource : fCachedResources) {
		std::string resourceName = resource.second->Name();
		resourceName.append("(");
		resourceName.append(strresource(resource.second->Type()));
		resourceName.append(")");
		list.push_back(resourceName);
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
		uint32 key = resource->Key();
		if (resource->Release()) {
			auto iterator = fCachedResources.find(key);
			if (iterator != fCachedResources.end())
				fCachedResources.erase(key);
			delete resource;
			//std::cout << " and is now 0. Resource deleted";
		} /*else
			std::cout << " and is now " << resource->RefCount();
		std::cout << "." << std::endl;*/
	}
}


std::string
ResourceManager::GetFullPath(std::string name, uint16 location)
{
	//std::cout << "ResourceManager::GetFullPath(" << name << ", 0x";
	//std::cout << std::hex << location << ")" << std::endl;

	Storage::Path pathName(fResourcesPath);
	if (pathName.InitCheck() != 0)
		throw std::runtime_error("Invalid path");

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
ResourceManager::_LoadResource(const KeyResEntry &entry)
{
	const int bifIndex = RES_BIF_INDEX(entry.key);

	if (fDebugLevel > 0) {
		std::cout << kComponentName << "LoadResource(";
		std::cout << entry.name.CString() << ", " << strresource(entry.type);
		std::cout << ")" << std::endl;
	}
	const KeyFileEntry* fileEntry = fKeyDB->GetBIF(bifIndex);
	Archive *archive = fArchives[fileEntry->name];
	if (archive == NULL) {
		std::string fullPath = GetFullPath(fileEntry->name, fileEntry->location);
		if (fDebugLevel > 0) {
			std::cout << "\t-> Loading archive '" << fullPath << "'... ";
			std::flush(std::cout);
		}
		archive = Archive::Create(fullPath.c_str());
		if (archive == NULL) {
			if (fDebugLevel > 0)
				std::cout << RED("FAILED!") << std::endl;
			return NULL;
		}
		if (fDebugLevel > 0)
			std::cout << GREEN("OK!") << std::endl;
		fArchives[fileEntry->name] = archive;
	}

	Resource *resource = Resource::Create(entry.name, entry.type,
										entry.key, archive);
	if (resource == NULL) {
		if (fDebugLevel > 0)
			std::cout << RED(kComponentName) << RED("FAILED Loading resource!") << std::endl;
		return NULL;
	}

	resource->Acquire();
	fCachedResources[resource->Key()] = resource;
	if (fDebugLevel > 0) {
		std::cout << "\t-> Resource " << entry.name.CString();
		std::cout << " (" << strresource(entry.type) << ") ";
		std::cout << "loaded correctly!" << std::endl;
	}
	return resource;
}


Resource*
ResourceManager::_LoadResourceFromOverride(const KeyResEntry& entry)
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
ResourceManager::_LoadResourceFromOverride(const KeyResEntry& entry,
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
		return NULL;
	}

	resource->Acquire();
	fCachedResources[resource->Key()] = resource;

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
	fKeyDB->PrintResources(type);
}


void
ResourceManager::PrintBIFs()
{
	fKeyDB->PrintBIFs();
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
ResourceManager::_FindResource(const KeyResEntry &entry)
{
	auto iter = fCachedResources.find(entry.key);
	if (iter == fCachedResources.end())
		return NULL;

	return iter->second;
}



void
ResourceManager::TryEmptyResourceCache()
{
#if 0
	// TODO: This causes font resources (amongs others) to be unloaded
	// when they are still used. Need to fix resource unloading
	auto it = fCachedResources.begin();
	while (it != fCachedResources.end()) {
		if ((*it)->RefCount() == 1) {
			//std::cout << "deleting resource " << (*it)->Name() << std::endl;
			delete *it;
		} else {
			//std::cout << "releasing resource " << (*it)->Name() << "(still used)" << std::endl;
			(*it)->Release();
		}

		it = fCachedResources.erase(it);
	}
#endif
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
	TLKEntry* entry = GetTLKEntry(i);
	if (entry != NULL) {
		text = entry->text;
		delete entry;
	}
	return text;
}


TLKEntry*
IDTable::GetTLKEntry(uint32 i)
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
	std::string string = sAniSnd->StringForID(i);
	// lines are like
	// 28928 = MBAS     CGAMEANIMATIONTYPE_BASILISK
	// so truncate at first space
	return string.substr(0, string.find(' '));
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
IDTable::ActionName(uint32 i)
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
