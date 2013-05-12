#ifndef __IDSRESOURCE_H
#define __IDSRESOURCE_H

#include "Resource.h"

#include <map>
#include <string>

class IDSResource : public Resource {
public:
	IDSResource(const res_ref &name);
	virtual bool Load(Archive *archive, uint32 key);

	std::string ValueFor(uint32 id);

	typedef std::map<uint32, std::string> string_map;

protected:
	bool _IsEncrypted();

	string_map fMap;
};



#endif // __IDSRESOURCE_H
