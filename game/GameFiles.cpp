/*
 * GameFiles.cpp - see GameFiles.h
 */

#include "GameFiles.h"

#include "ResManager.h"

#include <filesystem>
#include <strings.h>

std::string
FindGameFile(const std::string& relativePath)
{
	namespace fs = std::filesystem;

	fs::path current(gResManager->ResourcesPath());

	size_t start = 0;
	while (start <= relativePath.size()) {
		size_t end = relativePath.find('/', start);
		if (end == std::string::npos)
			end = relativePath.size();
		const std::string component = relativePath.substr(start, end - start);
		start = end + 1;
		if (component.empty())
			continue;

		std::error_code error;
		bool found = false;
		for (const fs::directory_entry& entry : fs::directory_iterator(current, error)) {
			const std::string name = entry.path().filename().string();
			if (strcasecmp(name.c_str(), component.c_str()) == 0) {
				current = entry.path();
				found = true;
				break;
			}
		}
		if (!found)
			return "";
	}
	return current.string();
}
