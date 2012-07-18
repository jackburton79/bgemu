#include "Archive.h"
#include "BIFArchive.h"
#include "DirectoryArchive.h"
#include "PlainFileArchive.h"
#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <new>

/* static */
Archive *
Archive::Create(const char *path)
{
	Archive* archive = NULL;
	try {
		/*if (extension(path) == NULL) { // TODO: Not so nice
			printf("Archive::Create(): DirectoryArchive\n");
			return new DirectoryArchive(path);
		} else */if (!strcasecmp(extension(path), ".bif"))
			archive = new BIFArchive(path);
		else {
			archive = new PlainFileArchive(path);
		}
	} catch (...) {
		printf("TArchive::Create(): Exception thrown!\n");
		delete archive; // TODO: We leak the archive ?
	}
	return archive;
}
