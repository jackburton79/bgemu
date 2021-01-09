#ifndef __RESOURCE_MANAGER_H
#define __RESOURCE_MANAGER_H

#include "KEYResource.h"
#include "Path.h"
#include "Resource.h"
#include "IETypes.h"

#include <list>
#include <map>
#include <string>
#include <vector>

extern const char *kKeyResource;
extern const char *kDialogResource;

class Archive;
class TWODAResource;
class ARAResource;
class BAMResource;
class BCSResource;
class BMPResource;
class CHUIResource;
class CREResource;
class DLGResource;
class IDSResource;
class ITMResource;
class MOSResource;
class MVEResource;
class KEYResource;
class SPLResource;
class TISResource;
class TLKResource;
class VVCResource;
class WEDResource;
class WMAPResource;
class ResourceManager {
public:	
	static bool Initialize(const char *path);
	static void Destroy();

	void SetDebug(int level);

	bool ResourceExists(const res_ref& ref, uint16 type) const;
	
	KEYResource*	GetKEY(const char *name);
	TLKResource*	GetTLK(const char *name);
	TWODAResource*	Get2DA(const res_ref& name);
	BAMResource*	GetBAM(const res_ref& name);
	BCSResource*	GetBCS(const res_ref& name);
	BMPResource*	GetBMP(const res_ref& name);
	CHUIResource*	GetCHUI(const res_ref& name);
	CREResource*	GetCRE(const res_ref& name);
	DLGResource*	GetDLG(const res_ref& name);
	IDSResource*	GetIDS(const res_ref& name);
	ITMResource*	GetITM(const res_ref& name);
	TISResource*	GetTIS(const res_ref& name);
	WEDResource*	GetWED(const res_ref& name);
	ARAResource*	GetARA(const res_ref& name);
	MOSResource*	GetMOS(const res_ref& name);
	MVEResource*	GetMVE(const res_ref& name);
	VVCResource*	GetVVC(const res_ref& name);
	WMAPResource*	GetWMAP(const res_ref& name);
	SPLResource*	GetSPL(const res_ref& name);

	Resource *GetResource(const char* fullName);
	Resource *GetResource(const res_ref &name, uint16 type);

	void GetCachedResourcesList(StringList& list);

	void ReleaseResource(Resource *resource);
	void TryEmptyResourceCache(bool force = false);
	
	void PrintResources(int32 type = -1);
	void PrintBIFs();
	
	static std::string HeightMapName(const char *name);
	static std::string LightMapName(const char *name);
	static std::string SearchMapName(const char *name);

private:
	ResourceManager(const char* path);
	~ResourceManager();

	const char *ResourcesPath() const;

	const KeyResEntry *_GetKeyRes(const res_ref &name, uint16 type) const;

	Resource *_FindResource(const KeyResEntry &entry);
	Resource *_LoadResource(const KeyResEntry &entry);
	Resource* _LoadResourceFromOverride(const KeyResEntry& entry);
	Resource* _LoadResourceFromOverride(const KeyResEntry& entry, const char* overridePath);

	std::string GetFullPath(std::string name, uint16 location);

	void _TryEmptyResourceCache();

	typedef std::map<ref_type, KeyResEntry *> resource_map;
	typedef std::vector<KeyFileEntry *> bif_vector;
	typedef std::map<std::string, Archive *> archive_map;

	Path fResourcesPath;

	bif_vector fBifs;
	resource_map fResourceMap;
	std::list<Resource*> fCachedResources;
	archive_map fArchives;

	bool fDebugLevel;
};

extern ResourceManager *gResManager;

TLKResource* Dialogs();
class TLKEntry;
class IDTable {
public:
	static std::string GetDialog(uint32 i);
	static TLKEntry* GetTLK(uint32 i);

	static std::string	RandomColorAt(uint32 i);
	static std::string	AlignmentAt(uint32 i);
	static std::string	GeneralAt(uint32 i);
	static std::string	AnimationAt(uint32 i);
	static std::string	AniSndAt(uint32 i);

	static std::string	GenderAt(uint32 i);
	static uint32		GenderID(std::string);

	static std::string	RaceAt(uint32 i);
	static std::string	ClassAt(uint32 i);
	static std::string	SpecificAt(uint32 i);

	static std::string	TriggerName(uint32 i);
	static uint32		TriggerID(std::string name);

	static std::string 	ActionAt(uint32 i);

	static std::string 	ObjectAt(uint32 i);
	static uint32 		ObjectID(std::string string);

	static std::string	EnemyAllyAt(uint32 i);
	static uint32 		EnemyAllyValue(std::string);

	static std::string 	GameTimeAt(uint32 i);
	static std::string 	TimeAt(uint32 i);
	static std::string 	TimesOfDayAt(uint32 i);
};


#endif
