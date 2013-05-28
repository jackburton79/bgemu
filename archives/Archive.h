#ifndef __ARCHIVE_ADDON_H
#define __ARCHIVE_ADDON_H

#include "IETypes.h"

class MemoryStream;
class Archive {
public:
	virtual ~Archive() {};

	virtual void EnumEntries() = 0;
	
	virtual MemoryStream* ReadResource(res_ref& ref,
			const uint32& key,
			const uint16& type) = 0;

	static Archive *Create(const char *filename);
};


#endif
