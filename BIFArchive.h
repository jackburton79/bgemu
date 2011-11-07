#ifndef __BIFARCHIVE_H
#define __BIFARCHIVE_H

#include "Archive.h"

struct resource_info;
struct tileset_info;
class Stream;
class BIFArchive : public Archive {
public:
	BIFArchive(const char *fileName);
	virtual ~BIFArchive();

	virtual void EnumEntries();
	virtual bool GetResourceInfo(resource_info &info,
					uint16 index) const;
	virtual bool GetTilesetInfo(tileset_info &info,
						uint16 index) const;
	virtual ssize_t ReadAt(uint32 offset,
				void *buffer, uint32 size) const;

private:

	ssize_t _ExtractFileBlock(Stream &source, Stream &dest);

	Stream *fStream;

	uint32 fNumEntries;
	uint32 fNumTilesetEntries;
	uint32 fCatalogOffset;
	uint32 fTileEntriesOffset;
};

#endif
