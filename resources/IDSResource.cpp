#include "IDSResource.h"
#include "MemoryStream.h"
#include "Utils.h"

#include <algorithm>
#include <stdlib.h>

static const uint8 kEncryptionKey[64] = {
	0x88, 0xa8, 0x8f, 0xba, 0x8a, 0xd3, 0xb9, 0xf5,
	0xed, 0xb1, 0xcf, 0xea, 0xaa, 0xe4, 0xb5, 0xfb,
	0xeb, 0x82, 0xf9, 0x90, 0xca, 0xc9, 0xb5, 0xe7,
	0xdc, 0x8e, 0xb7, 0xac, 0xee, 0xf7, 0xe0, 0xca,
	0x8e, 0xea, 0xca, 0x80, 0xce, 0xc5, 0xad, 0xb7,
	0xc4, 0xd0, 0x84, 0x93, 0xd5, 0xf0, 0xeb, 0xc8,
	0xb4, 0x9d, 0xcc, 0xaf, 0xa5, 0x95, 0xba, 0x99,
	0x87, 0xd2, 0x9d, 0xe3, 0x91, 0xba, 0x90, 0xca
};


class EncryptedStream : public MemoryStream {
public:
	EncryptedStream(MemoryStream *stream);
	virtual uint8 ReadByte();

private:
	int fKeySize;
};


EncryptedStream::EncryptedStream(MemoryStream *stream)
	:
	MemoryStream(stream->Size() - 2),
	fKeySize(64)
{
	stream->ReadAt(2, Data(), Size());
}


/* virtual */
uint8
EncryptedStream::ReadByte()
{
	int pos = Position();
	uint8 byteRead = MemoryStream::ReadByte();
	byteRead ^= kEncryptionKey[pos % fKeySize];
	return byteRead;
}


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

	if (_IsEncrypted()) {
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

	return true;
}


std::string
IDSResource::ValueFor(uint32 id)
{
	return fMap[id];
}


bool
IDSResource::_IsEncrypted()
{
	int32 pos = fData->Position();
	fData->Seek(0, SEEK_SET);

	bool encrypted = (fData->ReadByte() == 0xFF)
		&& (fData->ReadByte() == 0xFF);

	fData->Seek(pos, SEEK_SET);

	return encrypted;
}
