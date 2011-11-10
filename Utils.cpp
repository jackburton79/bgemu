#include "Path.h"
#include "Types.h"
#include "Utils.h"

#include <assert.h>
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
	// TODO: Fix this mess
	TPath path(filename);
	std::string leaf = path.Leaf();
	std::string resName = filename;
	TPath parent;
	if (path.GetParent(&parent) == 0) {
		resName = parent.Leaf();
		resName.append("/").append(leaf);
	}

	printf("Requested file %s:\n", filename);
	FILE *handle = NULL;
	char **names = NULL;
	int num = get_case_names(resName.c_str(), &names);
	for (int i = 0; i < num; i++) {
		TPath fullName;
		if (parent.GetParent(&fullName) == 0)
			fullName.Append(names[i]);
		else
			fullName = names[i];

		printf("\t%d: trying %s...", i, fullName.Path());
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
free_case_names(char **names, int num)
{
	for (int i = 0; i < num; i++)
		free(names[i]);
	free(names);
}


int
get_case_names(const char *name, char ***_names)
{
	const int num = 13;
	char **names = *_names = (char**)malloc(num * sizeof(char**));

	int length = strlen(name);

	// Change names
	// data/AREA0001E.bif
	names[0] = strdup(name);

	// data/AREA0001e.bif
	names[1] = strdup(name);
	char *ext = (char *)::extension(names[1]) - 1;
	std::transform(ext, &names[1][length], ext, ::tolower);

	// data/AREA0001E.BIF
	names[2] = strdup(name);
	std::transform((char*)extension(names[2]), &names[2][length],
			(char*)extension(names[2]), ::toupper);

	// data/AREA0001e.BIF
	names[3] = strdup(names[1]);
	std::transform((char*)extension(names[3]), &names[3][length],
				(char*)extension(names[3]), ::toupper);

	names[4] = strdup(names[0]);
	std::transform((char*)extension(names[4]), &names[4][length],
					(char*)extension(names[4]), ::toupper);

	//char *leaf = ::leaf(names[0]);


	for (int32 i = 0; i < 4; i++) {
		names[4 + i] = strdup(names[i]);
		names[4 + i][0] = (char)toupper(names[4 + i][0]);
		names[8 + i] = strdup(names[i]);
		std::transform((char*)extension(names[8 + i]), &names[8 + i][length],
						(char*)extension(names[8 + i]), ::toupper);

	}

	names[num - 1] = strdup(name);
	std::transform((char*)names[num - 1], &names[num - 1][length],
			(char*)names[num - 1], ::toupper);

	return num;
}


void
check_objects_size()
{
	assert(sizeof(polygon) == 18);
	assert(sizeof(animation) == 76);
}

