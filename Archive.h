#ifndef __ARCHIVE_ADDON_H
#define __ARCHIVE_ADDON_H

#include <Types.h>


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

class TArchive {
public:
	virtual ~TArchive() {};

	virtual void EnumEntries() = 0;
	
	virtual bool GetResourceInfo(resource_info &info,
			uint16 index) const = 0;
	virtual bool GetTilesetInfo(tileset_info &info,
				uint16 index) const = 0;
	virtual ssize_t ReadAt(uint32 offset,
			void *buffer, uint32 size) const = 0;

	static TArchive *Create(const char *filename);
};


#endif
