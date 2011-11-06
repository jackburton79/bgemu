#include "Path.h"
#include "Types.h"
#include "Utils.h"

#include <assert.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <algorithm>
#include <limits.h>

void
path_dos_to_unix(char *path)
{
	char *c;
	while ((c = strchr(path, '\\')) != NULL)
		*c = '/';
}


FILE*
fopen_case(const char *filename, const char *flags)
{
	// TODO: Fix this mess
	TPath path(filename);
	std::string leaf = path.Leaf();
	TPath parent;
	path.GetParent(&parent);
	std::string resName = parent.Leaf();
	resName.append("/").append(leaf);

	printf("Requested file %s:\n", filename);
	FILE *handle = NULL;
	char **names = NULL;
	int num = get_case_names(resName.c_str(), &names);
	for (int i = 0; i < num; i++) {
		TPath fullName;
		parent.GetParent(&fullName);
		fullName.Append(names[i]);
		printf("\ttrying %s...", fullName.Path());
		handle = fopen(fullName.Path(), flags);
		if (handle == NULL)
			printf("NOT FOUND!\n");
		else
			break;
	}
	free_case_names(names, num);

	if (handle != NULL)
		printf("FOUND!\n");

	return handle;
}


const char *
extension(const char *path)
{
	if (path == NULL)
		return NULL;

	return strrchr(path, '.');
}


static void
extension_toupper(char *name)
{
	char *ptr = (char *)extension(name);
	if (ptr == NULL)
		return;
	ptr++;
	char c;
	while ((c = *ptr) != '\0')
		*ptr++ = (char)toupper(c);
}


static void
name_toupper(char *name)
{
	char *ptr = name;
	if (ptr == NULL)
		return;

	char c;
	while ((c = *ptr) != '\0')
		*ptr++ = (char)toupper(c);
}


void
free_case_names(char **names, int num)
{
	for (int i = 0; i < num; i++)
		free(names[i]);
	free(names);
}


int
get_case_names(const char *name, char ***_names)
{
	char **names = *_names = (char**)malloc(6 * sizeof(char**));

	// data/area0001.bif
	names[0] = strdup(name);

	// data/area0001.BIF
	names[1] = strdup(names[0]);
	extension_toupper(names[1]);

	// Data/area0001.bif
	names[2] = strdup(names[0]);
	names[2][0] = (char)toupper(name[0]);

	// Data/area0001.BIF
	names[3] = strdup(names[2]);
	extension_toupper(names[3]);

	// data/AREA0001.BIF
	names[4] = strdup(names[0]);
	name_toupper(names[4]);

	// data/AREA0001.BIF
	names[5] = strdup(names[2]);
	name_toupper(names[5]);

	return 6;
}


void
check_objects_size()
{
	assert(sizeof(polygon) == 18);
	assert(sizeof(animation) == 76);
}

