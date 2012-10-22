#include "BamResource.h"
#include "GraphicsEngine.h"
#include "Path.h"
#include "ResManager.h"
#include "Utils.h"

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include <algorithm>
#include <limits.h>

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
	size_t where = 0;
	TPath newPath("/");

	char leaf[PATH_MAX];
	char* start = (char*)path.Path() + 1;
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
	/*if (handle != NULL)
		printf("FOUND!\n");
	else
		printf("NOT FOUND!\n");
*/
	return handle;
}


const char*
leaf(const char *path)
{
	if (path == NULL)
		return NULL;

	return strrchr(path, '/');
}


const char*
extension(const char* path)
{
	if (path == NULL)
		return NULL;

	const char* point = path + strlen(path) - 4;
	if (*point != '.')
		return NULL;
	return point;
}


// TODO: Move RenderString to its own file, or a different file
static uint32
cycle_num_for_char(char c)
{
	return (int)c - 1;
}


void
RenderString(std::string string, const res_ref& fontRes,
		uint32 flags, Bitmap* bitmap)
{
	BAMResource* fontResource = gResManager->GetBAM(fontRes);
	RenderString(string, fontResource, flags, bitmap);
	gResManager->ReleaseResource(fontResource);
}


void
RenderString(std::string string, BAMResource* fontResource,
		uint32 flags, Bitmap* bitmap)
{
	Frame* frames = new Frame[string.length()];
	std::string::iterator i = string.begin();
	uint32 totalWidth = 0;
	uint16 maxHeight = 0;
	int numFrames = 0;
	while (i != string.end()) {
		uint32 cycleNum = cycle_num_for_char(*i);
		frames[numFrames] = fontResource->FrameForCycle(cycleNum, 0);
		totalWidth += frames[numFrames].rect.w;
		maxHeight = std::max(frames[numFrames].rect.h, maxHeight);
		numFrames++;
		i++;
	}

	GFX::rect rect = { 0, 0, 0, 0 };
	if (flags & IE::LABEL_JUSTIFY_BOTTOM)
		rect.y = bitmap->Height() - maxHeight;
	else if (flags & IE::LABEL_JUSTIFY_TOP)
		rect.y = 0;
	else
		rect.y = (bitmap->Height() - maxHeight) / 2;

	if (flags & IE::LABEL_JUSTIFY_CENTER)
		rect.x = (bitmap->Width() - totalWidth) / 2;
	else if (flags & IE::LABEL_JUSTIFY_RIGHT)
		rect.x = bitmap->Width() - totalWidth;

	for (int f = 0; f < numFrames; f++) {
		rect.w = frames[f].rect.w;
		rect.h = frames[f].rect.h;

		GraphicsEngine::BlitBitmap(frames[f].bitmap,
				NULL, bitmap, &rect);
		rect.x += frames[f].rect.w;
		GraphicsEngine::DeleteBitmap(frames[f].bitmap);
	}
	delete[] frames;
}
