#ifndef __RESOURCE_MANAGER_H
#define __RESOURCE_MANAGER_H

#include "KeyFile.h"
#include "Path.h"
#include "Resource.h"
#include "Types.h"

#include <map>
#include <string>
#include <vector>

class TArchive;
class AREAResource;
class BAMResource;
class BMPResource;
class TISResource;
class WEDResource;
class ResourceManager {
public:
	ResourceManager();
	~ResourceManager();
	
	void SetResourcesPath(const char *path);
	void ParseKeyFile(const char *fileName);	
	
	BAMResource *GetBAM(const res_ref &name);
	BMPResource *GetBMP(const res_ref &name);
	TISResource *GetTIS(const res_ref &name);
	WEDResource *GetWED(const res_ref &name);
	AREAResource *GetAREA(const res_ref &name);
	Resource *GetResource(const res_ref &name, uint16 type);
	void ReleaseResource(Resource *resource);
	
	void PrintResources();
	void PrintBIFs();
	
	static std::string HeightMapName(const char *name);
	static std::string LightMapName(const char *name);
	static std::string SearchMapName(const char *name);

private:
	const char *ResourcesPath() const;
	KeyResEntry *GetKeyRes(const res_ref &name, uint16 type);
	Resource *FindResource(KeyResEntry &entry);
	Resource *LoadResource(KeyResEntry &entry);

	std::string GetFullPath(std::string name, uint16 location);

	void _TryEmptyResourceCache();

	typedef std::map<ref_type, KeyResEntry *> resource_map;
	typedef std::vector<KeyFileEntry *> bif_vector;
	typedef std::map<std::string, TArchive *> archive_map;

	TPath fResourcesPath;

	bif_vector fBifs;
	resource_map fResourceMap;
	std::vector<Resource *> fCachedResources;
	archive_map fArchives;
	

};

extern ResourceManager *gResManager;

#endif
