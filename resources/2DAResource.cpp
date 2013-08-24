/*
 * TWODAResource.cpp
 *
 *  Created on: 01/ott/2012
 *      Author: stefano
 */

#include "2DAResource.h"
#include "EncryptedStream.h"
#include "MemoryStream.h"

#define TWODA_SIGNATURE "2DA "
#define TWODA_VERSION_1 "V1.0"

TWODAResource::TWODAResource(const res_ref& name)
	:
	Resource(name, RES_2DA)
{
}


TWODAResource::~TWODAResource()
{

}


/* virtual */
bool
TWODAResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (IsEncrypted()) {
		EncryptedStream *newStream =
				new EncryptedStream(fData);
		ReplaceData(newStream);
	}

	if (CheckSignature(TWODA_SIGNATURE, true)) {
		fData->Seek(4, SEEK_CUR);
	}

	if (CheckVersion(TWODA_VERSION_1, true)) {
		fData->Seek(4, SEEK_CUR);
	}

	//fData->Seek(8, SEEK_SET);

	char string[256];
	fData->ReadLine(string, sizeof(string));
		// skip line TODO: Handle

	std::cout << "*** " << string << " ***" << std::endl;

	fData->ReadLine(string, sizeof(string)); // skip ??? Why!?

	std::cout << string << std::endl;
	fData->ReadLine(string, sizeof(string)); // headers

	std::cout << string << std::endl;
	std::map<int, std::string> headerMap;
	char *name = strtok(string, " ");
	if (name != NULL) {
		headerMap[0] = name;
		int i = 0;
		while (true) {
			char *stringValue = strtok(NULL, " \n\r");
			if (stringValue == NULL)
				break;
			i++;
			headerMap[i] = stringValue;
		}

		i = 0;
		while (fData->ReadLine(string, sizeof(string)) != NULL) {
			char *name = strtok(string, " ");
			if (name == NULL)
				continue;

			while (true) {
				char *stringValue = strtok(NULL, " \n\r");
				if (stringValue == NULL)
					break;

				fMap[std::make_pair(name, headerMap[i])] = stringValue;
				i++;
			}
		}
	}

	return true;
}


/* virtual */
void
TWODAResource::Dump()
{
	std::map<std::pair<std::string, std::string>, std::string>::const_iterator i;
	for (i = fMap.begin(); i != fMap.end(); i++) {
		std::cout << i->first.first << ", " << i->first.second;
		std::cout << " = " << i->second << std::endl;
	}
}


std::string
TWODAResource::ValueFor(const char* rowValue, const char* columnValue)
{
	return fMap[std::make_pair(rowValue, columnValue)];
}
