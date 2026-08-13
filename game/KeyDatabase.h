/*
 * ResourceDB.h
 *
 *  Created on: 13 ago 2026
 */

#pragma once

#include "KEYResource.h"

#include <map>
#include <vector>

class KeyDatabase {
public:
	KeyDatabase();
	~KeyDatabase();

	bool Load(const char* path);

	const KeyResEntry* Find(const ref_type& id) const;
	const KeyFileEntry* GetBIF(uint32 index) const;

	void PrintResources(int32 type);
	void PrintBIFs();

private:
	std::map<ref_type, KeyResEntry*> fResourceMap;
	std::vector<KeyFileEntry*> fBifs;
};


