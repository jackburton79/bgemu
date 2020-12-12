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

/* static */
Resource*
TWODAResource::Create(const res_ref& name)
{
	return new TWODAResource(name);
}


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

	if (CheckSignature(TWODA_SIGNATURE)) {
		fData->Seek(4, SEEK_CUR);
	}

	if (CheckVersion(TWODA_VERSION_1)) {
		fData->Seek(4, SEEK_CUR);
	}

	//fData->Seek(8, SEEK_SET);

	char string[1024];
	fData->ReadLine(string, sizeof(string));
//	std::cout << "*** FIRST LINE: " << string << " ***" << std::endl;

	fData->ReadLine(string, sizeof(string));
	fDefaultValue = string;
//	std::cout << "*** SECOND LINE:" << string << std::endl;

	fData->ReadLine(string, sizeof(string)); // headers

//	std::cout << "*** THIRD LINE: " << string << std::endl;

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
TWODAResource::ValueFor(const char* rowValue, const char* columnValue) const
{
	const char* value = columnValue ? columnValue : "";
	//std::cout << "ValueFor:" << rowValue << " " << value << std::endl;
	//StringMap::iterator i = fMap.find(std::make_pair(rowValue, value));
	return fMap.at(std::make_pair(rowValue, value));
}


int32
TWODAResource::IntegerValueFor(const char* rowValue, const char* columnValue) const
{
	return ::strtol(ValueFor(rowValue, columnValue).c_str(), NULL, 0);
}


std::string
TWODAResource::ValueAt(int rowIndex, int columnIndex) const
{
	return fTable.at(rowIndex).at(columnIndex);
}


int32
TWODAResource::IntegerValueAt(int rowIndex, int columnIndex) const
{
	return ::strtol(fTable.at(rowIndex).at(columnIndex).c_str(), NULL, 0);
}


int32
TWODAResource::CountRows() const
{
	return fTable.size();
}


int32
TWODAResource::CountColumns() const
{
	return fTable.at(0).size();
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
	if (stringValue == NULL)
		return;

	fRowHeaders.push_back(stringValue);
	int i = 0;
	StringList row;
	while ((stringValue = ::strtok(NULL, " \t")) != NULL) {
		//if (strcmp(stringValue, "") == 0)
		//	continue;
		row.push_back(stringValue);
		fMap[std::make_pair(fRowHeaders.back(), fColumnHeaders[i])] = stringValue;
		i++;
	}
	fTable.push_back(row);
}
