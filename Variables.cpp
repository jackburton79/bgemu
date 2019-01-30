/*
 * Variables.cpp
 *
 *      Author: Stefano Ceccherini
 */


#include "Variables.h"

#include <iostream>

Variables::Variables()
{
}


void
Variables::Set(const char* name, int32 value)
{
	fVariables[name] = value;
}


int32
Variables::Get(const char* name) const
{
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

