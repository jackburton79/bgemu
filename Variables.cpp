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
	Print();
#endif
}


int32
Variables::Get(const char* name) const
{
#if DEBUG_VARIABLES
	Print();
#endif
	VariablesMap::const_iterator i = fVariables.find(name);		
	if (i != fVariables.end())
		return i->second;
	return 0;
}


void
Variables::Print() const
{
	VariablesMap::const_iterator i;
	for (i = fVariables.begin(); i != fVariables.end(); i++) {
		std::cout << i->first << "=" << i->second << std::endl;	
	}
}

