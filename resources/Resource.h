#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "IETypes.h"
#include "Referenceable.h"

struct res_ref;
class Archive;
class ResourceManager;
class Stream;
class Resource : public Referenceable {
public:
	friend class ResourceManager;

	virtual bool Load(Archive *archive, uint32 key);
	virtual Resource* Clone();

	virtual void Dump();
	void DumpToFile(const char *fileName);

	uint32 Key() const;
	uint16 Type() const;
	const char* Name() const;
	
protected:
	Resource(const res_ref &name, const uint16 &type);
	virtual ~Resource();

	static Resource* Create(const res_ref &name, const uint16& type);
	static Resource* Create(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive);

	bool CheckSignature(const char *signature, bool dontWorry = false);
	bool CheckVersion(const char *version, bool dontWorry = false);
	
	bool ReplaceData(Stream *stream);
	void DropData();

	bool IsEncrypted();

	Stream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;
};


#endif // __RESOURCE_H

