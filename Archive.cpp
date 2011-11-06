#include "Archive.h"
#include "BIFArchive.h"
#include "DirectoryArchive.h"
#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <new>

/* static */
TArchive *
TArchive::Create(const char *path)
{
	try {
		if (!strcasecmp(extension(path), ".bif"))
			return new TBIFArchive(path);
		else if (path[strlen(path) - 1] == '/') {
			printf("TArchive::Create(): DirectoryArchive\n");
			return new DirectoryArchive(path);
		} else {
			printf("TArchive::Create(): CRASHBANG!\n");
			return NULL;
		}
	} catch (...) {
		printf("TArchive::Create(): Exception thrown!\n");
		return NULL;
	}
}
