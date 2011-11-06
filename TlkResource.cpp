#include "TlkResource.h"

#include <assert.h>

#define SIGNATURE "TLK V1  "

TlkResource::TlkResource(uint8 *data, uint32 size, uint32 key)
	:
	TResource(data, size, key)
{
	char signature[9];
	ReadAt(0, signature, 8);
	signature[8] = '\0';
	assert(strcmp(signature, SIGNATURE) == 0);

	ReadAt(8, fNumEntries);
	ReadAt(12, fDataOffset);
}


bool
TlkResource::FindString(uint32 strref, char **buffer)
{
	if (strref < 0 || strref >= fNumEntries)
		return false;
	
	*buffer = NULL;
	return true;
}
