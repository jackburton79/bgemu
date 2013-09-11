#include "IDSResource.h"
#include "EncryptedStream.h"
#include "MemoryStream.h"
#include "Utils.h"

#include <algorithm>
#include <stdlib.h>
#include <stdexcept>


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

	if (IsEncrypted()) {
		EncryptedStream *newStream =
				new EncryptedStream(fData);
		ReplaceData(newStream);
	}

	uint32 numEntries = 0;
	char string[256];
	if (fData->ReadLine(string, sizeof(string)) != NULL)
		numEntries = atoi(string);

	(void)(numEntries);

	// PFFFT! just ignore the number of entries,
	// since most IDS files contain an empty first line
	while (fData->ReadLine(string, sizeof(string)) != NULL) {
		char *stringID = strtok(string, " ");
		if (stringID == NULL)
			continue;
		char *stringValue = strtok(NULL, "\n\r");
		if (stringValue == NULL)
			continue;
		char *finalValue = trim(stringValue);
		char *rest = NULL;
		uint32 id = strtoul(stringID, &rest, 0);

		std::transform(finalValue, finalValue + strlen(finalValue),
					finalValue, ::toupper);
		fMap[id] = finalValue;
	}

	DropData();

	return true;
}


void
IDSResource::Dump()
{
	for (string_map::const_iterator i = fMap.begin(); i != fMap.end(); i++) {
		std::cout << i->first << " = " << i->second << std::endl;
	}
}


std::string
IDSResource::ValueFor(uint32 id) const
{
	string_map::const_iterator i = fMap.find(id);
	if (i == fMap.end())
		throw std::runtime_error("IDSResource::ValueFor(): no such id");
	return i->second;
}


uint32
IDSResource::ValueFor(std::string string) const
{
	string_map::const_iterator i;
	for (i = fMap.begin(); i != fMap.end(); i++) {
		if (i->second == string)
			return i->first;
	}

	throw std::runtime_error("IDSResource::ValueFor(): no such value");
}
