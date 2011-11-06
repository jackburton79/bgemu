#include "DirectoryArchive.h"


DirectoryArchive::DirectoryArchive(const char *path)
{
	fDir = opendir(path);
	if (fDir != NULL) {

	}
}


DirectoryArchive::~DirectoryArchive()
{
	if (fDir != NULL)
		closedir(fDir);
}


/* virtual */
void
DirectoryArchive::EnumEntries()
{
	if (fDir == NULL)
		return;

	dirent *entry = NULL;
	while ((entry = readdir(fDir)) != NULL) {
		printf("%s\n", entry->d_name);
	}
	rewinddir(fDir);
}


/* virtual */
bool
DirectoryArchive::GetResourceInfo(resource_info &info,
			uint16 index) const
{
	return false;
}


/* virtual */
bool
DirectoryArchive::GetTilesetInfo(tileset_info &info,
			uint16 index) const
{
	return false;
}


/* virtual */
ssize_t
DirectoryArchive::ReadAt(uint32 offset, void *buffer, uint32 size) const
{
	return -1;
}
