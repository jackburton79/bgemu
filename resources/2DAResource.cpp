/*
 * TWODAResource.cpp
 *
 *  Created on: 01/ott/2012
 *      Author: stefano
 */

#include "MemoryStream.h"
#include "2DAResource.h"

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

	if (CheckSignature(TWODA_SIGNATURE)) {
		fData->Seek(4, SEEK_CUR);
	}

	if (CheckVersion(TWODA_VERSION_1)) {
		fData->Seek(4, SEEK_CUR);
	}

	//fData->Seek(8, SEEK_SET);

	char string[256];
	fData->ReadLine(string, sizeof(string));
		// skip line TODO: Handle
	printf("*** %s ***\n", string);

	fData->ReadLine(string, sizeof(string)); // skip ??? Why!?

	fData->ReadLine(string, sizeof(string)); // headers

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


std::string
TWODAResource::ValueFor(const char* rowValue, const char* columnValue)
{
	return fMap[std::make_pair(rowValue, columnValue)];
}
