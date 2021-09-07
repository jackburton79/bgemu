/*
 * Variables.cpp
 *
 *      Author: Stefano Ceccherini
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
	VariablesMap::const_iterator i = fVariables.find(name);		
	if (i != fVariables.end())
		return i->second;
	return 0;
}


/* static */
void
Variables::GetScopeName(const char* variable, std::string& varScope, std::string& varName)
{
	varScope.append(variable, 6);
	varName.append(&variable[6]);
}


void
Variables::Print(const char* variable) const
{
	int32 value = 0;
	VariablesMap::const_iterator i = fVariables.find(variable);		
	if (i != fVariables.end())
		value = i->second;
	std::cout << "VARIABLE " << variable << " value is " << value << std::endl;	
}


void
Variables::PrintAll() const
{
	VariablesMap::const_iterator i;
	for (i = fVariables.begin(); i != fVariables.end(); i++) {
		std::cout << i->first << "=" << i->second << std::endl;	
	}
}

