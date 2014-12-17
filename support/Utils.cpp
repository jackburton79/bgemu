#include "Path.h"
#include "Utils.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>


const char*
trim(char* string)
{
	while (isspace(*string))
		string++;

	char* endOfString = string;
	while (!isspace(*endOfString) && *endOfString != '\0')
		endOfString++;

	*endOfString = '\0';

	return string;
}


void
path_dos_to_unix(char* path)
{
	char* c;
	while ((c = strchr(path, '\\')) != NULL)
		*c = '/';
}


FILE*
fopen_case(const char* filename, const char* flags)
{
	assert(filename != NULL);
	assert(strlen(filename) > 1);

	TPath normalizedFileName(filename);
	std::string newPath("/");
	char* start = (char*)normalizedFileName.Path() + 1;
	size_t where = 0;
	while ((where = strcspn(start, "/")) > 0) {
		std::string leaf;
		leaf.append(start, where);
		DIR* dir = opendir(newPath.c_str());
		if (dir != NULL) {
			dirent *entry = NULL;
			while ((entry = readdir(dir)) != NULL) {
				if (!strcasecmp(entry->d_name, leaf.c_str())) {
					if (newPath != "/")
						newPath.append("/");
					newPath.append(entry->d_name);
					break;
				}
			}
			closedir(dir);
		}
		start += where + 1;
	}

	FILE* handle = NULL;
	if (!strcasecmp(filename, newPath.c_str()))
		handle = fopen(newPath.c_str(), flags);

	return handle;
}


const char*
leaf(const char *path)
{
	if (path == NULL)
		return NULL;

	const char* leafPointer = strrchr(path, '/');
	return leafPointer != NULL ? leafPointer : path;
}


const char*
extension(const char* path)
{
	if (path == NULL)
		return NULL;

	const char* point = path + strlen(path) - 4;
	if (point == NULL || *point != '.')
		return NULL;
	return point;
}

