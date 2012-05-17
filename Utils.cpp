#include "Path.h"
#include "Types.h"
#include "Utils.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include <algorithm>
#include <limits.h>

char *
trim(char *string)
{
	char *returnString = string;
	while (isspace(*returnString))
		returnString++;

	char *ptr = returnString;
	while (!isspace(*ptr) && *ptr != '\0')
		ptr++;

	*ptr = '\0';

	return returnString;
}


void
path_dos_to_unix(char *path)
{
	char *c;
	while ((c = strchr(path, '\\')) != NULL)
		*c = '/';
}


FILE *
fopen_case(const char *filename, const char *flags)
{
	TPath path(filename);

	bool first = true;
	size_t where = 0;
	TPath newPath("");
	char base[256];
	char leaf[256];
	char *start = (char*)path.Path();
	while ((where = strcspn(start, "/\0")) > 0) {
		strncpy(base, start, where);
		base[where] = 0;
		if (first) {
			newPath.Append(base);
			first = false;
		}
		char* newStart = start + where + 1;
		size_t w = strcspn(newStart, "/\0");
		if (w == 0)
			break;

		strncpy(leaf, newStart, w);
		leaf[w] = 0;

		DIR *dir = opendir(newPath.Path());
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
		start = newStart;
	}

	printf("%s...", newPath.Path());
	FILE *handle = fopen(newPath.Path(), flags);
	if (handle != NULL)
		printf("FOUND!\n");
	else
		printf("NOT FOUND!\n");

	return handle;
}


const char *
leaf(const char *path)
{
	if (path == NULL)
		return NULL;

	return strrchr(path, '/');
}


const char *
extension(const char *path)
{
	if (path == NULL)
		return NULL;

	return strrchr(path, '.');
}


void
check_objects_size()
{
	assert(sizeof(polygon) == 18);
	assert(sizeof(animation) == 76);
}

