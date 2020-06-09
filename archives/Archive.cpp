#include "Archive.h"
#include "BIFArchive.h"
#include "DirectoryArchive.h"
#include "IETypes.h"
#include "FileArchive.h"
#include "MemoryStream.h"
#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <new>

/* static */
Archive*
Archive::Create(const char *path)
{
	Archive* archive = NULL;
	const char* ext = extension(path);
	try {
		if (ext == NULL) { // TODO: Not so nice
			archive = new DirectoryArchive(path);
		} else if (!strcasecmp(ext, "bif"))
			archive = new BIFArchive(path);
		else {
			archive = new PlainFileArchive(path);
		}
	} catch (...) {
		delete archive;
		archive = NULL;
	}
	return archive;
}


