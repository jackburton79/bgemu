/*
 * ResourceDB.h
 *
 *  Created on: 13 ago 2026
 */

#pragma once

#include "KEYResource.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

struct RefTypeHash {
	size_t operator()(
		const ref_type& id) const
	{
		std::string key = id.name.CString();

		std::transform(key.begin(), key.end(), key.begin(), ::toupper);

		return std::hash<std::string>()(key) ^ std::hash<uint16>()(id.type);
	}
};

class KeyDatabase {
public:
	KeyDatabase();
	~KeyDatabase();

	bool Load(const char* path);
	void Dispose();

	const KeyResEntry* Find(const ref_type& id) const;
	const KeyFileEntry* GetBIF(uint32 index) const;

	int32 CountResources() const;
	int32 CountBIFs() const;

	void PrintResources(int32 type);
	void PrintBIFs();

private:
	std::unordered_map<ref_type, KeyResEntry*, RefTypeHash> fResourceMap;
	std::vector<KeyFileEntry*> fBifs;
};


