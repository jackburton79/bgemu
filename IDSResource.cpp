#include "IDSResource.h"
#include "MemoryStream.h"

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

	for (uint32 e = 0; e < numEntries; e++) {
		fData->ReadLine(string, sizeof(string));
		char *stringID = strtok(string, " ");
		char *stringValue = strtok(NULL, " ");
		char *rest = NULL;
		int32 id = strtol(stringID, &rest, 16);
		fMap[id] = stringValue;
	}

	DropData();

	return true;
}


const char *
IDSResource::ValueFor(int32 id)
{
	return fMap[id].c_str();
}
