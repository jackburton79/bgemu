#include "DirectoryArchive.h"
#include "Path.h"
#include "PlainFileArchive.h"



DirectoryArchive::DirectoryArchive(const char *path)
	:
	fPath(path)
{
	fDir = opendir(path);
	if (fDir == NULL) {
		printf("Failed to open directory");
		throw -1;
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
MemoryStream*
DirectoryArchive::ReadResource(res_ref& ref,
		const uint32& key, 	const uint16& type)
{
	if (fDir == NULL)
		return NULL;

	MemoryStream* stream = NULL;
	dirent *entry = NULL;
	while ((entry = readdir(fDir)) != NULL) {
		if (!strncasecmp(ref.name, entry->d_name, 8)) {
			TPath filePath = fPath.Path();
			filePath.Append(entry->d_name);
			PlainFileArchive archive(filePath.Path());
			stream = archive.ReadResource(ref, key, type);
			break;
		}
	}
	rewinddir(fDir);
	return stream;
}

