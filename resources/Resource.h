#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "IETypes.h"
#include "Referenceable.h"

#include <Stream.h>

#include <map>

struct res_desc;
struct res_ref;
class Archive;
class ResourceManager;
class Resource : public Referenceable {
public:
	Resource(const res_ref &name, const uint16 &type);
	virtual ~Resource();
	
	virtual bool Load(Archive *archive, uint32 key);

	void DumpToFile(const char *fileName);

	const uint32 Key() const;
	const uint16 Type() const;
	const char* Name() const;
	
	static Resource* Create(const res_ref &name, uint16 type);
	static Resource* Create(const res_ref &name, 	const uint16& type,
			const uint32& key, Archive* archive);
	
protected:
	bool CheckSignature(const char *signature, bool dontWorry = false);
	bool CheckVersion(const char *version, bool dontWorry = false);
	
	bool ReplaceData(Stream *stream);
	void DropData();

	//static bool InitResourcesDescription();
	//static std::map<uint16, res_desc> *sResourcesDescription;

	Stream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;
};


#endif
