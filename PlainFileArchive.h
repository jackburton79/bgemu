#ifndef __PLAINFILEARCHIVE_H
#define __PLAINFILEARCHIVE_H

#include "Archive.h"

class FileStream;
class PlainFileArchive : public Archive {
public:
	PlainFileArchive(const char *path);
	virtual ~PlainFileArchive();

	virtual void EnumEntries();

	virtual bool GetResourceInfo(resource_info &info,
			uint16 index) const;
	virtual bool GetTilesetInfo(tileset_info &info,
			uint16 index) const;
	virtual ssize_t ReadAt(uint32 offset,
			void *buffer, uint32 size) const;

private:
	FileStream *fFile;
};

#endif // __PLAINFILEARCHIVE_H
