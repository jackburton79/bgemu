#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "IETypes.h"
#include "Referenceable.h"
#include "Stream.h"


struct res_ref;
class Archive;
class MemoryStream;
class ResourceManager;
class Resource : public Referenceable {
public:
	Resource(const res_ref &name, const uint16 &type);
	virtual ~Resource();
	
	virtual bool Load(Archive *archive, uint32 key);

	void DumpToFile(const char *fileName);

	const uint32 Key() const;
	const uint16 Type() const;
	
	static Resource *Create(const res_ref &name, uint16 type);
	static const char* Extension(uint16 type);
	
protected:
	bool CheckSignature(const char *signature, bool dontWorry = false);
	bool CheckVersion(const char *version, bool dontWorry = false);
	
	bool ReplaceData(MemoryStream *stream);
	void DropData();

	MemoryStream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;
};


#endif
