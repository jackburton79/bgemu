/*
 * Variables.h
 *
 *      Author: Stefano Ceccherini
 */

#ifndef __VARIABLES_H
#define __VARIABLES_H

#include "SupportDefs.h"

#include <map>
#include <string>
#include <vector>


class Variables {
public:
	Variables();
	void Set(const char* name, int32 value);
	int32 Get(const char* name) const;
	static void GetNameAndScope(const char* variable, std::string& varScope, std::string& varName);

	void Print(const char* name) const;
	void PrintAll() const;

	// All (name, value) pairs, for GamResource to persist to a save file.
	std::vector<std::pair<std::string, int32>> All() const;

private:
	typedef std::map<std::string, uint32> VariablesMap;
	VariablesMap fVariables;
};

#endif /* __VARIABLES_H */
