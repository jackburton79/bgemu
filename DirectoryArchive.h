#ifndef __DIRECTORYARCHIVE_H
#define __DIRECTORYARCHIVE_H

#include "Archive.h"

#include <dirent.h>

class DirectoryArchive : public TArchive {
public:
	DirectoryArchive(const char *path);
	virtual ~DirectoryArchive();

	virtual void EnumEntries();

	virtual bool GetResourceInfo(resource_info &info,
			uint16 index) const;
	virtual bool GetTilesetInfo(tileset_info &info,
			uint16 index) const;
	virtual ssize_t ReadAt(uint32 offset,
			void *buffer, uint32 size) const;
private:
	DIR *fDir;
};

#endif // __DIRECTORYARCHIVE_H
