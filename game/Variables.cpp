/*
 * Variables.cpp
 *
 *      Author: Jackburton
 */


#include "Variables.h"

#include "Core.h"
#include "Object.h"

#include <cctype>
#include <iostream>

#define DEBUG_VARIABLES 0


// The real engine's variable names are case-insensitive and ignore
// embedded spaces (IESDP: "the variable name itself is case-insensitive"
// - matches GemRB's Variables::MyHashKey()). Normalizing at the one
// chokepoint both Set()/Get() go through - rather than at every call
// site - makes this automatic for LOCALS (Object::fVariables) and
// GLOBAL/area-scoped (Core::Vars()) alike, since both are just Variables
// instances.
static std::string
_NormalizeKey(const char* name)
{
	std::string key;
	for (const char* p = name; *p != '\0'; p++) {
		if (*p != ' ')
			key += (char)tolower((unsigned char)*p);
	}
	return key;
}


Variables::Variables()
{
}


void
Variables::Set(const char* name, int32 value)
{
	fVariables[_NormalizeKey(name)] = value;
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
	const auto &v = fVariables.find(_NormalizeKey(name));
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


/* static */
int32
Variables::GetScoped(Object* sender, const char* prefixedName)
{
	std::string scope, name;
	GetNameAndScope(prefixedName, scope, name);
	if (scope.compare("LOCALS") == 0)
		return sender != NULL ? sender->GetVariable(name.c_str()) : 0;
	return Core::Get()->Vars().Get(name.c_str());
}


/* static */
void
Variables::SetScoped(Object* sender, const char* prefixedName, int32 value)
{
	std::string scope, name;
	GetNameAndScope(prefixedName, scope, name);
	if (scope.compare("LOCALS") == 0) {
		if (sender != NULL)
			sender->SetVariable(name.c_str(), value);
	} else {
		Core::Get()->Vars().Set(name.c_str(), value);
	}
}


void
Variables::Print(const char* variable) const
{
	int32 value = 0;
	const auto& v = fVariables.find(_NormalizeKey(variable));
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

