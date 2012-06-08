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
	try {
		if (!strcasecmp(extension(path), ".bif"))
			return new BIFArchive(path);
		else if (path[strlen(path) - 1] == '/') {
			printf("Archive::Create(): DirectoryArchive\n");
			return new DirectoryArchive(path);
		} else {
			return new PlainFileArchive(path);
		}
	} catch (...) {
		printf("TArchive::Create(): Exception thrown!\n");
	}
	return NULL;
}
