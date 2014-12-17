#include "Path.h"
#include "Utils.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <string.h>


char*
trim(char* string)
{
	char* returnString = string;
	while (isspace(*returnString))
		returnString++;

	char* ptr = returnString;
	while (!isspace(*ptr) && *ptr != '\0')
		ptr++;

	*ptr = '\0';

	return returnString;
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

	TPath path(filename, NULL);
	TPath newPath("/");

	char leaf[PATH_MAX];
	char* start = (char*)path.Path() + 1;
	size_t where = 0;
	while ((where = strcspn(start, "/")) > 0) {
		char* newStart = start;
		strncpy(leaf, newStart, where);
		leaf[where] = 0;

		DIR* dir = opendir(newPath.Path());
		if (dir != NULL) {
			dirent *entry = NULL;
			while ((entry = readdir(dir)) != NULL) {
				if (!strcasecmp(entry->d_name, leaf)) {
					newPath.Append(entry->d_name);
					break;
				}
			}
			closedir(dir);
		}
		start = newStart + where + 1;
	}

	if (strcasecmp(filename, newPath.Path()))
		return NULL;

	FILE* handle = fopen(newPath.Path(), flags);

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

