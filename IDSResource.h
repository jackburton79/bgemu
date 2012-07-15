#ifndef __IDSRESOURCE_H
#define __IDSRESOURCE_H

#include "Resource.h"

#include <map>
#include <string>

class IDSResource : public Resource {
public:
	IDSResource(const res_ref &name);
	virtual bool Load(Archive *archive, uint32 key);

	const char *ValueFor(uint32 id);

	typedef std::map<uint32, std::string> string_map;

protected:
	bool _IsEncrypted();

	string_map fMap;
};


class WriteIDSResource : public IDSResource {
public:
	WriteIDSResource(const res_ref& name);
	bool AddValue(uint32 id, std::string value);

};


#endif // __IDSRESOURCE_H
