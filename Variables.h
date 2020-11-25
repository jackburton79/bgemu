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


class Variables {
public:
	Variables();
	void Set(const char* name, int32 value);
	int32 Get(const char* name) const;
	void Print(const char* name) const;
	void PrintAll() const;
private:
	typedef std::map<std::string, uint32> VariablesMap;
	VariablesMap fVariables;
};

#endif /* __VARIABLES_H */
