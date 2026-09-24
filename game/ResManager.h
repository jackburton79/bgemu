#ifndef __RESOURCE_MANAGER_H
#define __RESOURCE_MANAGER_H

#include "KEYResource.h"
#include "Path.h"
#include "Resource.h"
#include "IETypes.h"

#include <list>
#include <map>
#include <string>
#include <unordered_map>
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
class GamResource;
class IDSResource;
class ITMResource;
class KeyDatabase;
class MOSResource;
class MVEResource;
class KEYResource;
class PLTResource;
class SPLResource;
class STOResource;
class TISResource;
class TLKResource;
class VVCResource;
class WAVResource;
class WEDResource;
class WMAPResource;
class ResourceManager {
public:	
	static bool Initialize(const char *path);
	static void Destroy();

	void SetDebug(int level);

	bool ResourceExists(const res_ref& ref, uint16 type) const;
	std::vector<res_ref> ResourceNames(uint16 type) const;

	KEYResource*	GetKEY(const char *name);
	TLKResource*	GetTLK(const char *name);
	TWODAResource*	Get2DA(const res_ref& name);
	BAMResource*	GetBAM(const res_ref& name);
	BCSResource*	GetBCS(const res_ref& name);
	BMPResource*	GetBMP(const res_ref& name);
	CHUIResource*	GetCHUI(const res_ref& name);
	CREResource*	GetCRE(const res_ref& name);
	DLGResource*	GetDLG(const res_ref& name);
	GamResource*	GetGAM(const res_ref& name);
	IDSResource*	GetIDS(const res_ref& name);
	ITMResource*	GetITM(const res_ref& name);
	TISResource*	GetTIS(const res_ref& name);
	WEDResource*	GetWED(const res_ref& name);
	ARAResource*	GetARA(const res_ref& name);
	MOSResource*	GetMOS(const res_ref& name);
	MVEResource*	GetMVE(const res_ref& name);
	VVCResource*	GetVVC(const res_ref& name);
	WAVResource*	GetWAV(const res_ref& name);
	WMAPResource*	GetWMAP(const res_ref& name);
	PLTResource*	GetPLT(const res_ref& name);
	SPLResource*	GetSPL(const res_ref& name);
	STOResource*	GetSTO(const res_ref& name);

	Resource *GetResource(const char* fullName);
	Resource *GetResource(const res_ref &name, uint16 type);

	// Registers a resource built at runtime (not backed by any KEY/BIF
	// or override file) under (name, type), so later GetResource() /
	// Get*() calls for that pair return it - used for a created-from-
	// scratch player CRE. Takes over one reference (Acquire()d here);
	// replacing an existing injection releases the old one.
	void InjectResource(const res_ref& name, uint16 type, Resource* resource);

	void GetCachedResourcesList(StringList& list);

	void ReleaseResource(Resource *resource);
	void TryEmptyResourceCache();
	
	void PrintResources(int32 type = -1);
	void PrintBIFs();
	
	static std::string HeightMapName(const char *name);
	static std::string LightMapName(const char *name);
	static std::string SearchMapName(const char *name);

	// The game's directory.
	const char *ResourcesPath() const;

private:
	ResourceManager(const char* path);
	~ResourceManager();

	Resource *_FindResource(const KeyResEntry &entry);
	Resource *_LoadResource(const KeyResEntry &entry);
	Resource* _LoadResourceFromOverride(const KeyResEntry& entry);
	Resource* _LoadResourceFromOverride(const KeyResEntry& entry, const char* overridePath);

	std::string GetFullPath(std::string name, uint16 location);

	void _TryEmptyResourceCache();

	Storage::Path fResourcesPath;
	KeyDatabase* fKeyDB;

	std::unordered_map<uint32, Resource*> fCachedResources;
	std::map<std::pair<std::string, uint16>, Resource*> fInjectedResources;
	std::map<std::string, Archive *> fArchives;

	bool fDebugLevel;
};

extern ResourceManager *gResManager;

TLKResource* Dialogs();
class TLKEntry;
class IDTable {
public:
	static std::string GetDialog(int32 i);
	static TLKEntry* GetTLKEntry(int32 i);

	static std::string	RandomColorAt(int32 i);
	static std::string	AlignmentAt(int32 i);
	static std::string	GeneralAt(int32 i);
	static std::string	AnimationAt(int32 i);
	static std::string	AniSndAt(int32 i);

	static std::string	GenderAt(int32 i);
	static int32		GenderID(std::string);

	static std::string	RaceAt(int32 i);
	static std::string	ClassAt(int32 i);
	static std::string	SpecificAt(int32 i);

	// The properly localized (TLK strref) display text for a race/class/
	// alignment id, unlike RaceAt()/ClassAt()/AlignmentAt() above (which
	// return the raw, always-English RACE.IDS/CLASS.IDS/ALIGNMENT.IDS
	// symbol - correct for debug output, wrong for UI). Real BG1/BG2
	// hardcode this mapping in their own EXE rather than shipping it as
	// a 2DA, so the table backing these is GemRB's own "unhardcoded"
	// races.2da/classes.2da/aligns.2da (identical between both games for
	// every id they share), not a loaded game resource. Empty string for
	// an id outside that table (an exotic/modded creature) - callers
	// fall back to the raw IDS-symbol form for those.
	// ClassName() only covers a single class or a true multi-class (the
	// CRE's own class byte already names the combined row, e.g.
	// FIGHTER_CLERIC) - a dual-classed character or a kit (which need
	// the second, previous class/kit and IE_TITLE1, GemRB's
	// GetActorClassTitle()) isn't modeled and falls back the same way.
	static std::string	RaceName(uint32 raceID);
	static std::string	AlignmentName(uint8 alignmentValue);
	static std::string	ClassName(uint32 classID);

	static std::string	TriggerName(int32 i);
	static int32		TriggerID(std::string name);

	static std::string 	ActionName(int32 i);
	static int32		ActionID(std::string name);

	static std::string 	ObjectAt(int32 i);
	static int32 		ObjectID(std::string string);

	static std::string	EnemyAllyAt(int32 i);
	static int32 		EnemyAllyValue(std::string);

	static std::string 	GameTimeAt(int32 i);
	static std::string 	TimeAt(int32 i);
	static std::string 	TimesOfDayAt(int32 i);
};


#endif
