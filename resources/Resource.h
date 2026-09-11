#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "IETypes.h"
#include "Referenceable.h"

#include <vector>

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

	// The resource's current raw bytes (fData), verbatim - reflects any
	// in-place mutation done through this resource's own setters (e.g. a
	// CREResource's inventory/HP/spellbook writes), unlike Dump()/
	// DumpToFile() which are diagnostic text dumps, not a byte-exact
	// re-serialization. Used to embed a live CRE's current state
	// somewhere else (see ARAResource::WriteToFile()'s embedded-CRE
	// checkpointing).
	void RawData(std::vector<uint8>& out) const;

	uint32 Key() const;
	uint16 Type() const;
	std::string Name() const;

protected:
	Resource(const res_ref &name, const uint16 &type);
	virtual ~Resource();

	static Resource* Create(const res_ref &name, const uint16& type);
	static Resource* Create(const res_ref& name, const uint16& type,
		const uint32& key, Archive* archive);

	bool CheckSignature(const char *signature);
	bool CheckVersion(const char *version);
	
	bool ReplaceData(Stream *stream);
	void DropData();

	bool IsEncrypted();

	Stream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;
};


#endif // __RESOURCE_H

