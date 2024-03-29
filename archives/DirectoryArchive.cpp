#include "DirectoryArchive.h"
#include "FileArchive.h"
#include "Path.h"

#include <iostream>
#include <errno.h>
#include <string>

DirectoryArchive::DirectoryArchive(const char *path)
	:
	fPath(path)
{
	// TODO: This should work case insensitively
#if 0
	std::cout << "DirectoryArchive: " << path << std::endl;
#endif
	fDir = ::opendir(path);
	if (fDir == NULL) {
		throw std::runtime_error(::strerror(errno));
	}
}


DirectoryArchive::~DirectoryArchive()
{
	if (fDir != NULL)
		::closedir(fDir);
}


/* virtual */
void
DirectoryArchive::EnumEntries()
{
	if (fDir == NULL)
		return;

	dirent *entry = NULL;
	while ((entry = ::readdir(fDir)) != NULL) {
		std::cout << entry->d_name << std::endl;
	}
	::rewinddir(fDir);
}


/* virtual */
MemoryStream*
DirectoryArchive::ReadResource(res_ref& ref,
		const uint32& key, const uint16& type)
{
	if (fDir == NULL)
		return NULL;

	MemoryStream* stream = NULL;
	dirent *entry = NULL;
	while ((entry = ::readdir(fDir)) != NULL) {
		std::string resourceName = ref.CString();
		resourceName.append(".").append(res_extension(type));
		if (!strcasecmp(resourceName.c_str(), entry->d_name)) {
			Storage::Path filePath = fPath.String();
			filePath.Append(entry->d_name);
			PlainFileArchive archive(filePath.String());
			stream = archive.ReadResource(ref, key, type);
			break;
		}
	}
	::rewinddir(fDir);
	return stream;
}
