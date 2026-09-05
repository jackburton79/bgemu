/*
 * Variables.cpp
 *
 *      Author: Jackburton
 */


#include "Variables.h"

#include <iostream>

#define DEBUG_VARIABLES 0

Variables::Variables()
{
}


void
Variables::Set(const char* name, int32 value)
{
	fVariables[name] = value;
#if DEBUG_VARIABLES
	std::cout << "SET ";
	Print(name);
#endif
}


int32
Variables::Get(const char* name) const
{
#if DEBUG_VARIABLES
	std::cout << "GET ";
	Print(name);
#endif
	const auto &v = fVariables.find(name);
	if (v != fVariables.end())
		return v->second;
	return 0;
}


/* static */
void
Variables::GetNameAndScope(const char* variable, std::string& varScope, std::string& varName)
{
	varScope.append(variable, 6);
	varName.append(&variable[6]);
}


void
Variables::Print(const char* variable) const
{
	int32 value = 0;
	const auto& v = fVariables.find(variable);
	if (v != fVariables.end())
		value = v->second;
	std::cout << "VARIABLE " << variable << " value is " << value << std::endl;	
}


void
Variables::PrintAll() const
{
	for (const auto &v: fVariables) {
		std::cout << v.first << "=" << v.second << std::endl;
	}
}


std::vector<std::pair<std::string, int32>>
Variables::All() const
{
	std::vector<std::pair<std::string, int32>> all;
	all.reserve(fVariables.size());
	for (const auto& v : fVariables)
		all.push_back(std::make_pair(v.first, (int32)v.second));
	return all;
}

