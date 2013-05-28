#ifndef __BIFARCHIVE_H
#define __BIFARCHIVE_H

#include "Archive.h"

struct base_info {
	uint32 key;
	uint32 offset;
};

struct resource_info : public base_info
{
	uint32 size;
	uint16 type;
	uint16 unk;
};

struct tileset_info : public base_info
{
	uint32 numTiles;
	uint32 tileSize;
	uint32 type;
};


class Stream;
class BIFArchive : public Archive {
public:
	BIFArchive(const char* fileName);
	virtual ~BIFArchive();

	virtual void EnumEntries();

	virtual MemoryStream* ReadResource(res_ref& name,
			const uint32& key,
			const uint16& type);

private:
	bool _GetResourceInfo(resource_info& info,
						uint16 index) const;
	bool _GetTilesetInfo(tileset_info& info,
							uint16 index) const;
	ssize_t ReadAt(uint32 offset,
					void* buffer, uint32 size) const;
	ssize_t _ExtractFileBlock(Stream& source, Stream& dest);

	Stream* fStream;

	uint32 fNumEntries;
	uint32 fNumTilesetEntries;
	uint32 fCatalogOffset;
	uint32 fTileEntriesOffset;
};

#endif
