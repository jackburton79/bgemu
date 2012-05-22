#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "Stream.h"
#include "IETypes.h"


struct res_ref;
class Archive;
class MemoryStream;
class ResourceManager;
class Resource {
public:
	Resource(const res_ref &name, const uint16 &type);
	virtual ~Resource();
	
	virtual bool Load(Archive *archive, uint32 key);

	void DumpToFile(const char *fileName);

	const uint32 Key() const;
	const uint16 Type() const;
	
	static Resource *Create(const res_ref &name, uint16 type);
	
protected:
	friend class ResourceManager;

	bool CheckSignature(const char *signature, bool dontWorry = false);
	bool CheckVersion(const char *version, bool dontWorry = false);
	
	bool ReplaceData(MemoryStream *stream);
	void DropData();

private:
	void _Acquire();
	bool _Release();

protected:
	MemoryStream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;

private:
	int32 fRefCount;
};


#endif
