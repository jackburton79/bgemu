/*
 * TWODAResource.cpp
 *
 *  Created on: 01/ott/2012
 *      Author: stefano
 */

#include "2DAResource.h"
#include "EncryptedStream.h"
#include "MemoryStream.h"

#include <cstdlib>

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

	char string[1024];
	fData->ReadLine(string, sizeof(string));
	//std::cout << "*** FIRST LINE: " << string << " ***" << std::endl;

	fData->ReadLine(string, sizeof(string));
	fDefaultValue = string;
	
	//std::cout << "*** SECOND LINE:" << string << std::endl;
	fData->ReadLine(string, sizeof(string)); // headers

	//std::cout << "*** THIRD LINE: " << string << std::endl;

	_HandleHeadersRow(string);

	while (fData->ReadLine(string, sizeof(string))) {
		_HandleContentRow(string);
	//	std::cout << string << "/////" << std::endl;
	}

	return true;
}


/* virtual */
void
TWODAResource::Dump()
{
	std::cout << "default value: " << fDefaultValue << std::endl;
	
	for (StringList::const_iterator c = fColumnHeaders.begin();
										c != fColumnHeaders.end(); c++) {
			std::cout << *c << " ";
	}
	std::cout << std::endl;
	for (StringList::const_iterator r = fRowHeaders.begin();
									r != fRowHeaders.end(); r++) {
		std::cout << *r << ": ";
		for (StringList::const_iterator c = fColumnHeaders.begin();
										c != fColumnHeaders.end(); c++) {
			std::cout << fMap[std::make_pair(*r, *c)] << ";";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}


std::string
TWODAResource::ValueFor(const char* rowValue, const char* columnValue)
{
	const char* value = columnValue ? columnValue : "";
	return fMap[std::make_pair(rowValue, value)];
}


uint32
TWODAResource::IntegerValueFor(const char* rowValue, const char* columnValue)
{
	return ::strtoul(ValueFor(rowValue, columnValue).c_str(), NULL, 0);
}


void
TWODAResource::_HandleHeadersRow(char* string)
{
	for (const char *stringValue = ::strtok(string, " \t");
		stringValue != NULL; stringValue = ::strtok(NULL, " \t")) {
		fColumnHeaders.push_back(stringValue);
	}
}

	
void
TWODAResource::_HandleContentRow(char* string)
{
	const char *stringValue = ::strtok(string, " \t");
	fRowHeaders.push_back(stringValue);
	int i = 0;
	while ((stringValue = ::strtok(NULL, " \t")) != NULL) {
		//if (strcmp(stringValue, "") == 0)
		//	continue;
		fMap[std::make_pair(fRowHeaders.back(), fColumnHeaders[i])] = stringValue;
		i++;
	}
}
