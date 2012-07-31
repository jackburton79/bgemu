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
class ARAResource;
class BAMResource;
class BCSResource;
class BMPResource;
class CREResource;
class IDSResource;
class MOSResource;
class MVEResource;
class KEYResource;
class TISResource;
class TLKResource;
class WEDResource;
class WMAPResource;
class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();
	
	bool Initialize(const char *path);

	KEYResource *GetKEY(const char *name);
	TLKResource *GetTLK(const char *name);
	BAMResource *GetBAM(const res_ref& name);
	BCSResource *GetBCS(const res_ref& name);
	BMPResource *GetBMP(const res_ref& name);
	CREResource *GetCRE(const res_ref& name);
	IDSResource *GetIDS(const res_ref& name);
	TISResource *GetTIS(const res_ref& name);
	WEDResource *GetWED(const res_ref& name);
	ARAResource *GetARA(const res_ref& name);
	MOSResource* GetMOS(const res_ref& name);
	MVEResource *GetMVE(const res_ref& name);
	WMAPResource* GetWMAP(const res_ref& name);

	void ReleaseResource(Resource *resource);
	void TryEmptyResourceCache();
	
	void PrintResources();
	void PrintBIFs();
	
	static std::string HeightMapName(const char *name);
	static std::string LightMapName(const char *name);
	static std::string SearchMapName(const char *name);

private:
	const char *ResourcesPath() const;
	KeyResEntry *_GetKeyRes(const res_ref &name, uint16 type);

	Resource *_GetResource(const res_ref &name, uint16 type);
	Resource *_FindResource(KeyResEntry &entry);
	Resource *_LoadResource(KeyResEntry &entry);
	Resource* _LoadResourceFromOverride(KeyResEntry& entry);

	std::string GetFullPath(std::string name, uint16 location);

	void _TryEmptyResourceCache();

	typedef std::map<ref_type, KeyResEntry *> resource_map;
	typedef std::vector<KeyFileEntry *> bif_vector;
	typedef std::map<std::string, Archive *> archive_map;

	TPath fResourcesPath;

	bif_vector fBifs;
	resource_map fResourceMap;
	std::list<Resource*> fCachedResources;
	archive_map fArchives;
};

extern ResourceManager *gResManager;

#endif
