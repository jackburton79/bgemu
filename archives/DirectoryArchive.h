#ifndef __DIRECTORYARCHIVE_H
#define __DIRECTORYARCHIVE_H

#include "Archive.h"
#include "Path.h"

#include <dirent.h>

namespace Storage {
	class Path;
};

class DirectoryArchive : public Archive {
public:
	DirectoryArchive(const char *path);
	virtual ~DirectoryArchive();

	virtual void EnumEntries();

	virtual MemoryStream* ReadResource(res_ref& ref,
				const uint32& key,
				const uint16& type);

private:
	DIR *fDir;
	Storage::Path fPath;
};

#endif // __DIRECTORYARCHIVE_H
