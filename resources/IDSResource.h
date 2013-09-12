#ifndef __IDSRESOURCE_H
#define __IDSRESOURCE_H

#include "Resource.h"

#include <map>
#include <string>

class IDSResource : public Resource {
public:
	IDSResource(const res_ref &name);
	virtual bool Load(Archive *archive, uint32 key);
	virtual void Dump();

	std::string StringForID(uint32 id) const;
	uint32 IDForString(std::string string) const;

	typedef std::map<uint32, std::string> string_map;

protected:
	bool _IsEncrypted();

	string_map fMap;
};



#endif // __IDSRESOURCE_H
