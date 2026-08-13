/*
 * KeyDatabase.cpp
 *
 *  Created on: 13 ago 2026
 *      Author: stefano.ceccherini@iacpud.local
 */

#include "KeyDatabase.h"

#include <memory>

#include "Archive.h"

KeyDatabase::KeyDatabase()
{
}


KeyDatabase::~KeyDatabase()
{
	Dispose();
}


void
KeyDatabase::Dispose()
{
	std::cout << "KeyDatabase: " << "Deleting resource_maps...";
	std::cout << std::endl;
	for (auto entry: fResourceMap) {
		delete entry.second;
	}

	std::cout << "KeyDatabase: " << "Deleting bifs maps...";
	std::cout << std::endl;
	for (auto entry: fBifs) {
		delete entry;
	}

	fResourceMap.clear();
	fBifs.clear();
}


bool
KeyDatabase::Load(const char* path)
{
	Dispose();

	std::unique_ptr<KEYResource> key(new KEYResource("KEY"));
	std::unique_ptr<Archive> archive(Archive::Create(path));

	// TODO: Throw an useful exception instead
	if (!key.get()->Load(archive.get(), 0))
		throw std::runtime_error("GetKey: cannot load key file!");

	const uint32 numBifs = key->CountFileEntries();
	for (uint32 b = 0; b < numBifs; b++) {
		if (KeyFileEntry* bif = key->GetFileEntryAt(b))
			fBifs.push_back(bif);
	}

	uint32 numResources = key->CountResourceEntries();
	for (uint32 c = 0; c < numResources; c++) {
		if (KeyResEntry *res = key->GetResEntryAt(c)) {
			fResourceMap[{res->name, res->type}] = res;
		}
	}

	return true;
}


const KeyResEntry*
KeyDatabase::Find(const ref_type& id) const
{
	const auto entry = fResourceMap.find(id);
	if (entry == fResourceMap.end())
		return NULL;
	return entry->second;
}



const KeyFileEntry*
KeyDatabase::GetBIF(uint32 index) const
{
	return fBifs.at(index);
}


int32
KeyDatabase::CountResources() const
{
	return fResourceMap.size();
}


int32
KeyDatabase::CountBIFs() const
{
	return fBifs.size();
}


void
KeyDatabase::PrintResources(int32 type)
{
	std::cout << "KeyDatabase: ";
	std::cout << "Listing " << fResourceMap.size();
	std::cout << " entries..." << std::endl;
	for (const auto& [key, value]: fResourceMap) {
		if (value == NULL) {
			std::cerr << "KeyResEntry is NULL. SHOULD NOT HAPPEN! ";
			std::cerr << key.name << " (";
			std::cerr << key.type << " )" << std::endl;
			continue;
		}
		if (type == -1 || type == value->type) {
			std::cout << value->name << " " << strresource(value->type);
			std::cout << ", " << fBifs[value->BIFIndex()]->name;
			std::cout << ", index " << value->BIFFileIndex();
			std::cout << std::endl;
		}
	}
}


void
KeyDatabase::PrintBIFs()
{
	for (const auto entry: fBifs) {
		std::cout << entry->name;
		std::cout << "\t" << std::hex << entry->location << std::endl;
	}
}
