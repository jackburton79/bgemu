#include "Path.h"
#include "Utils.h"

#include <assert.h>
#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

#include <iostream>

const char*
trim(char* string)
{
	while (isspace(*string))
		string++;

	ssize_t length = ::strlen(string);
	char* endOfString = string + length - 1;
	while (isspace(*endOfString))
		endOfString--;

	endOfString++;
	*endOfString  = '\0';

	return string;
}


void
path_dos_to_unix(char* path)
{
	char* c;
	while ((c = ::strchr(path, '\\')) != NULL)
		*c = '/';
}


FILE*
fopen_case(const char* filename, const char* flags)
{
	assert(filename != NULL);
	assert(::strlen(filename) > 1);

	Path normalizedFileName(filename);
	std::string newPath("/");
	char* start = (char*)normalizedFileName.String() + 1;
	char* end = start + ::strlen(normalizedFileName.String());
	size_t where = 0;
	while ((where = ::strcspn(start, "/")) > 0) {
		std::string leaf;
		leaf.append(start, where);
		DIR* dir = ::opendir(newPath.c_str());
		if (dir != NULL) {
			dirent *entry = NULL;
			while ((entry = ::readdir(dir)) != NULL) {
				if (!::strcasecmp(entry->d_name, leaf.c_str())) {
					if (newPath != "/")
						newPath.append("/");
					newPath.append(entry->d_name);
					break;
				}
			}
			::closedir(dir);
		}
		start += where + 1;
		if (start >= end)
			break;
	}

	FILE* handle = NULL;
	if (!::strcasecmp(filename, newPath.c_str()))
		handle = ::fopen(newPath.c_str(), flags);

	return handle;
}


const char*
extension(const char* path)
{
	if (path == NULL)
		return NULL;

	const char* point = ::strrchr(path, '.');
	if (point == NULL)
		return NULL;

	return point + 1;
}


size_t
get_unquoted_string(char* dest, char* source, size_t size)
{
	char* name = source;
	char* nameEnd = name + size;
	while (*name == '"')
		name++;
	while (*nameEnd != '"')
		nameEnd--;
	*nameEnd = '\0';
	::strncpy(dest, name, size);

	return nameEnd - source;
}

		
bool
is_bit_set(uint32 value, int bitPos)
{
	return value & (1 << bitPos);
}


void
set_bit(uint32& value, int bitPos)
{
	value |= (1 << bitPos);
}


void
clear_bit(uint32& value, int bitPos)
{
	value &= ~(1 << bitPos);
}


void
assert_size(size_t size, size_t controlValue)
{
	if (size != controlValue) {
		printf("size is %zu\n", size);
		assert(size == controlValue);
	}
}
