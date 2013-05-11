#include "2DAResource.h"
#include "Archive.h"
#include "AreaResource.h"
#include "BamResource.h"
#include "BmpResource.h"
#include "BCSResource.h"
#include "CHUIResource.h"
#include "CreResource.h"
#include "Core.h"
#include "FileStream.h"
#include "IDSResource.h"
#include "IETypes.h"
#include "KEYResource.h"
#include "MOSResource.h"
#include "MveResource.h"
#include "ResManager.h"
#include "Resource.h"
#include "TisResource.h"
#include "TLKResource.h"
#include "Utils.h"
#include "WedResource.h"
#include "WMAPResource.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <limits.h>

#define USE_OVERRIDE 0

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
static ResourceManager sManager;

static TLKResource* sDialogs;
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

const char *kKeyResource = "Chitin.key";
const char *kDialogResource = "dialog.tlk";


static
void
fill_anisnd_ids(WriteIDSResource* res)
{
	res->AddValue(0x2000, "MSIR");

	// Character animations
	res->AddValue(0x5000, "CHMB");
	res->AddValue(0x5001, "CEMB");
	res->AddValue(0x5002, "CDMB");
	res->AddValue(0x5003, "CIMB");
	res->AddValue(0x5010, "CHFB");
	res->AddValue(0x5011, "CEFB");
	res->AddValue(0x5012, "CDMB");
	res->AddValue(0x5013, "CIFB");
	res->AddValue(0x5100, "CHMB");
	res->AddValue(0x5101, "CEMB");
	res->AddValue(0x5102, "CDMB");
	res->AddValue(0x5103, "CIMB");
	res->AddValue(0x5110, "CHFB");
	res->AddValue(0x5111, "CEFB");
	res->AddValue(0x5112, "CDMB");
	res->AddValue(0x5113, "CIFB");
	res->AddValue(0x5200, "CHMW");
	res->AddValue(0x5201, "CEMW");
	res->AddValue(0x5202, "CDMW");
	res->AddValue(0x5210, "CHFW");
	res->AddValue(0x5211, "CEFW");
	res->AddValue(0x5212, "CDMW");
	res->AddValue(0x5300, "CHMB");
	res->AddValue(0x5301, "CEMB");
	res->AddValue(0x5302, "CDMB");
	res->AddValue(0x5303, "CIMB");
	res->AddValue(0x5310, "CHFB");
	res->AddValue(0x5311, "CEFB");
	res->AddValue(0x5312, "CDMB");
	res->AddValue(0x5313, "CIFB");
	res->AddValue(0x6000, "CHMB");
	res->AddValue(0x6001, "CEMB");
	res->AddValue(0x6002, "CDMB");
	res->AddValue(0x6003, "CIMB");
	res->AddValue(0x6004, "CDMB");
	//res->AddValue(0x6005, "CHMB");
	res->AddValue(0x6010, "CHFB");
	res->AddValue(0x6011, "CEFB");
	res->AddValue(0x6012, "CDMB");
	res->AddValue(0x6013, "CIFB");
	res->AddValue(0x6014, "CIFB");
	//res->AddValue(0x6015, "CHFB");
	res->AddValue(0x6100, "CHMB");
	res->AddValue(0x6101, "CEMB");
	res->AddValue(0x6102, "CDMB");
	res->AddValue(0x6103, "CIMB");
	res->AddValue(0x6104, "CDMB");
	res->AddValue(0x6105, "CHMB");
	res->AddValue(0x6110, "CHFB");
	res->AddValue(0x6111, "CEFB");
	res->AddValue(0x6112, "CDMB");
	res->AddValue(0x6113, "CIFB");
	res->AddValue(0x6114, "CDFB");
	//res->AddValue(0x6115, "CHFB");
	res->AddValue(0x6200, "CHMW");
	res->AddValue(0x6201, "CEMW");
	res->AddValue(0x6202, "CDMW");
	res->AddValue(0x6204, "CDMW");
	//res->AddValue(0x6205, "CHMW");
	res->AddValue(0x6210, "CHFW");
	res->AddValue(0x6211, "CEFW");
	res->AddValue(0x6212, "CDMW");
	res->AddValue(0x6214, "CDMW");
	//res->AddValue(0x6215, "CHFW");
	res->AddValue(0x6300, "CHMB");
	res->AddValue(0x6301, "CEMB");
	res->AddValue(0x6302, "CDMB");
	res->AddValue(0x6303, "CIMB");
	res->AddValue(0x6304, "CDMB");
	//res->AddValue(0x6305, "CHMB");
	res->AddValue(0x6310, "CHFB");
	res->AddValue(0x6311, "CEFB");
	res->AddValue(0x6312, "CDMB");
	//res->AddValue(0x6313, "CIFB");
	//res->AddValue(0x6314, "CIFB");
	//res->AddValue(0x6315 "CHFB");

	// rest
	res->AddValue(0x6404, "USAR1");
	res->AddValue(0x7001, "MOGR");
	res->AddValue(0x7400, "MDOG");
	res->AddValue(0x7a00, "MSPI");
	res->AddValue(0x7c01, "MTAS");
	res->AddValue(0x7e00, "MWER");
	res->AddValue(0x7f01, "MMIN");
	res->AddValue(0x7f02, "MBEH");
	res->AddValue(0x7f03, "MIMP");
	res->AddValue(0x7f09, "MSAH");
	res->AddValue(0x7f0c, "MKUO");
	res->AddValue(0x7f11, "MUMB");
	res->AddValue(0x7f14, "MGIT");
	res->AddValue(0x7f15, "MBES");
	res->AddValue(0x7f16, "AMOO");
	res->AddValue(0x7f17, "ARAB");
	res->AddValue(0x7f18, "ADER");
	res->AddValue(0x7f20, "AGRO");
	res->AddValue(0x7f21, "APHE");
	res->AddValue(0x7f27, "MDRO");
	res->AddValue(0x7f28, "MKUL");

	res->AddValue(0x8000, "MGNL");
	res->AddValue(0x8100, "MHOB");
	res->AddValue(0x9000, "MOGR");
	res->AddValue(0xb000, "ACOW");
	res->AddValue(0xb100, "AHRS");

	res->AddValue(0xb200, "NBEG");
	res->AddValue(0xb210, "NPRO");
	res->AddValue(0xb300, "NBOY");
	res->AddValue(0xb310, "NGRL");
	res->AddValue(0xb400, "NFAM");
	res->AddValue(0xb410, "NFAW");

	res->AddValue(0xca00, "NNOM");
	res->AddValue(0xca10, "NNOW");

	res->AddValue(0xc100, "ACAT");
	res->AddValue(0xc200, "ACHK");
	res->AddValue(0xc300, "ARAT");
	res->AddValue(0xc400, "ASQU");
	res->AddValue(0xc500, "ABAT");

	res->AddValue(0xc600, "NBEG");
	res->AddValue(0xc700, "NBOY");
	res->AddValue(0xc710, "NGRL");
	res->AddValue(0xc800, "NFAM");
	res->AddValue(0xc810, "NFAW");
	res->AddValue(0xc900, "NFAM"); // TODO
	res->AddValue(0xc910, "NFAW"); // TODO

	// Birds
	res->AddValue(0xd000, "AEAG");
	res->AddValue(0xd100, "AGUL");
	res->AddValue(0xd200, "AVUL");
	res->AddValue(0xd300, "ABIR");

	res->AddValue(0xe010, "METT");
}


ResourceManager::ResourceManager()
{
	// TODO: Move this elsewhere!
	IE::check_objects_size();
	gResManager = &sManager;
}


ResourceManager::~ResourceManager()
{
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
	gResManager->ReleaseResource(sDialogs);

	resource_map::iterator iter;
	for (iter = fResourceMap.begin(); iter != fResourceMap.end(); iter++) {
		delete iter->second;
	}
	
	bif_vector::iterator i;
	for (i = fBifs.begin(); i != fBifs.end(); i++) {
		delete *i;	
	}

	std::list<Resource *>::iterator it;
	for (it = fCachedResources.begin(); it != fCachedResources.end(); it++)
		delete *it;

	//TryEmptyResourceCache(true);

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
				if (!strcmp(res->name.name, "")) {
					printf("unnamed resource type %s, %d\n",
							strresource(res->type), res->key);
					// TODO: looks like in BG2 there is an unnamed resource
					// and this causes all kinds of problems. Investigate
					delete res;
					continue;
				}

				ref_type refType;
				refType.name = res->name;
				refType.type = res->type;
				fResourceMap[refType] = res;

			} else {
				std::cerr << "GetResEntryAt() failed" << std::endl;
				delete res;
			}
		}
	} catch (...) {
		std::cerr << "Error!!!" << std::endl;
		resourcesOk = false;
	}

	delete key;

	_LoadIDSResources();

	return resourcesOk;
}


void
ResourceManager::_LoadIDSResources()
{
	sDialogs = gResManager->GetTLK(kDialogResource);
	sAnimate = gResManager->GetIDS("ANIMATE");
	//std::cerr << "GetIDS(\"ANISND\")" << std::endl;
	sAniSnd = gResManager->GetIDS("ANISND");
	//std::cerr << "GetIDS(\"ANISND\") returned" << std::endl;
	// TODO: Improve
	if (sAniSnd == NULL) {
		// No AniSnd.ids file, let's use our own.
		//fGame = GAME_BALDURSGATE;
		std::cerr << "sAniSnd is NULL. Creating our own" << std::endl;
		WriteIDSResource* aniSnd = new WriteIDSResource("ANISND");
		fill_anisnd_ids((WriteIDSResource*)aniSnd);
		sAniSnd = aniSnd;
	} else {
		//fGame = GAME_BALDURSGATE2;
		//sAniSnd->DumpToFile("/home/stefano/anisnd.ids");
	}
	sGeneral = gResManager->GetIDS("GENERAL");
	sRaces = gResManager->GetIDS("RACE");
	sGenders = gResManager->GetIDS("GENDER");
	sClasses = gResManager->GetIDS("CLASS");
	sSpecifics = gResManager->GetIDS("SPECIFIC");
	sTriggers = gResManager->GetIDS("TRIGGER");
	sActions = gResManager->GetIDS("ACTION");
	sObjects = gResManager->GetIDS("OBJECT");
	sEA = gResManager->GetIDS("EA");
}


Resource *
ResourceManager::_GetResource(const res_ref &name, uint16 type)
{
	if (!strcmp(name.name, ""))
		return NULL;

	KeyResEntry *entry = _GetKeyRes(name, type);
	if (entry == NULL) {
		std::cerr << "ResourceManager::GetResource(";
		std::cerr << name.CString() << ", " << strresource(type);
		std::cerr << "): Resource does not exist!" << std::endl;
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

	return result;
}


KEYResource*
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
		//key->Acquire();
		delete archive;
	} catch (...) {
		return NULL;
	}
	return key;
}


TLKResource*
ResourceManager::GetTLK(const char* name)
{
	TLKResource* tlk = NULL;
	try {
		tlk = new TLKResource("TLK");
		std::string path = GetFullPath(name, LOC_ROOT);
		Archive *archive = Archive::Create(path.c_str());
		tlk->Load(archive, 0);
		tlk->Acquire();
		delete archive;
	} catch (...) {
		return NULL;
	}
	return tlk;
}


ARAResource*
ResourceManager::GetARA(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_ARA);
	return static_cast<ARAResource*>(resource);
}


BAMResource*
ResourceManager::GetBAM(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_BAM);
	return static_cast<BAMResource*>(resource);
}


BMPResource*
ResourceManager::GetBMP(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_BMP);
	return static_cast<BMPResource*>(resource);
}


BCSResource*
ResourceManager::GetBCS(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_BCS);
	return static_cast<BCSResource*>(resource);
}


CHUIResource*
ResourceManager::GetCHUI(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_CHU);
	return static_cast<CHUIResource*>(resource);
}


CREResource*
ResourceManager::GetCRE(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_CRE);
	return static_cast<CREResource*>(resource);
}


IDSResource*
ResourceManager::GetIDS(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_IDS);
	if (resource == NULL)
		std::cerr << "IDS is NULL" << std::endl;
	return static_cast<IDSResource*>(resource);
}


MOSResource*
ResourceManager::GetMOS(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_MOS);
	return static_cast<MOSResource*>(resource);
}


MVEResource*
ResourceManager::GetMVE(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_MVE);
	return static_cast<MVEResource*>(resource);
}


TISResource*
ResourceManager::GetTIS(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_TIS);
	return static_cast<TISResource*>(resource);
}


WEDResource*
ResourceManager::GetWED(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_WED);
	return static_cast<WEDResource*>(resource);
}


WMAPResource*
ResourceManager::GetWMAP(const res_ref& name)
{
	Resource* resource = _GetResource(name, RES_WMP);
	return static_cast<WMAPResource*>(resource);
}


void
ResourceManager::ReleaseResource(Resource* resource)
{
	if (resource != NULL)
		resource->Release();
}


std::string
ResourceManager::GetFullPath(std::string name, uint16 location)
{
	std::cout << "location: (" << std::hex << location << ")" << std::endl;
	TPath pathName(fResourcesPath);

	uint32 cd = GET_CD(location);
	if ((location & LOC_ROOT) == 0) {
		//if (IS_OVERRIDE(location))
		//	printf("\tshould check in override\n");
		// TODO: this represents the LIST of cd where
		// we can find the resource.
		// some resources exist on many cds.
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

	//printf("CD: 0x%x ", GET_CD(location));
	if (cd & LOC_CD1)
		std::cout << "1 ";
	if (cd & LOC_CD2)
		std::cout << "2 ";
	if (cd & LOC_CD3)
		std::cout << "3 ";
	if (cd & LOC_CD4)
		std::cout << "4 ";
	if (cd & LOC_CD5)
		std::cout << "5";

	std::cout << std::endl;

	pathName.Append(name.c_str(), false);

	return pathName.Path();
}


Resource*
ResourceManager::_LoadResource(KeyResEntry &entry)
{
	const int bifIndex = RES_BIF_INDEX(entry.key);
	const uint16& location = fBifs[bifIndex]->location;
	const char* archiveName = fBifs[bifIndex]->name;

	std::cout << "ResourceManager::LoadResource(";
	std::cout << entry.name.CString() << ", " << strresource(entry.type);
	std::cout << ")" << std::endl;

	Archive *archive = fArchives[archiveName];
	if (archive == NULL) {
		std::string fullPath = GetFullPath(archiveName, location);
		archive = Archive::Create(fullPath.c_str());
		std::cout << "Opening archive " << fullPath << "...";
		if (archive == NULL) {
			std::cout << "FAILED!" << std::endl;
			return NULL;
		}
		std::cout << "OK!" << std::endl;
		fArchives[archiveName] = archive;
	}

	Resource *resource = Resource::Create(entry.name, entry.type,
										entry.key, archive);
	if (resource == NULL) {
		std::cout << "FAILED Loading resource!" << std::endl;
		delete resource;
		return NULL;
	}

	resource->Acquire();
	fCachedResources.push_back(resource);

	std::cout << "Resource " << entry.name.CString();
	std::cout << " (" << strresource(entry.type) << ") ";
	std::cout << "loaded correctly!" << std::endl;

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

	std::cout << "Archive created" << std::endl;
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

	std::cout << "Resource " << entry.name << "(";
	std::cout << strresource(entry.type) << ")";
	std::cout << "loaded correctly from override!" << std::endl;

	delete dirArchive;
	return resource;
}


void
ResourceManager::PrintResources(int32 type)
{
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
		std::cout << iter - fBifs.begin() << "\t" << entry->name;
		std::cout << "\t" << std::hex << entry->location << endl;
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
	ref_type nameType = {name, type};
	resource_map::const_iterator iter = fResourceMap.find(nameType);
	if (iter == fResourceMap.end())
		return NULL;

	return iter->second;
}


void
ResourceManager::TryEmptyResourceCache(bool force)
{
	std::list<Resource*>::iterator it;
	for (it = fCachedResources.begin(); it != fCachedResources.end(); it++) {
		if (force || (*it)->RefCount() == 1)
			it = fCachedResources.erase(it);
	}
}


// global functions

TLKResource*
Dialogs()
{
	return sDialogs;
}


IDSResource*
GeneralIDS()
{
	return sGeneral;
}


IDSResource*
AnimateIDS()
{
	return sAnimate;
}


IDSResource*
AniSndIDS()
{
	return sAniSnd;
}


IDSResource*
RacesIDS()
{
	return sRaces;
}


IDSResource*
GendersIDS()
{
	return sGenders;
}


IDSResource*
ClassesIDS()
{
	return sClasses;
}


IDSResource*
SpecificIDS()
{
	return sSpecifics;
}


IDSResource*
TriggerIDS()
{
	return sTriggers;
}


IDSResource*
ActionIDS()
{
	return sActions;
}


IDSResource*
ObjectsIDS()
{
	return sObjects;
}


IDSResource*
EAIDS()
{
	return sEA;
}
