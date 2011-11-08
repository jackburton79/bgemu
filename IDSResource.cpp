#include "IDSResource.h"
#include "MemoryStream.h"
#include "Utils.h"

#include <stdlib.h>

IDSResource::IDSResource(const res_ref &name)
	:
	Resource(name, RES_IDS)
{
}


/* virtual */
bool
IDSResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	uint32 numEntries = 0;
	char string[128];
	if (fData->ReadLine(string, sizeof(string)) != NULL)
		numEntries = atoi(string);

	//printf("%s, %d entries\n", (const char*)fName, numEntries);

	// PFFFT! just ignore the number of entries,
	// since most IDS files contain an empty first line
	while (fData->ReadLine(string, sizeof(string)) != NULL) {
		//printf("line: *%s*\n", string);
		char *stringID = strtok(string, " ");
		char *stringValue = strtok(NULL, "\n\r");
		char *finalValue = trim(stringValue);
		char *rest = NULL;
		int32 id = strtol(stringID, &rest, 0);
		fMap[id] = finalValue;
	}

	DropData();

	return true;
}


const char *
IDSResource::ValueFor(uint32 id)
{
	return fMap[id].c_str();
}
