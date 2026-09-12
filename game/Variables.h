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


class Object;

class Variables {
public:
	Variables();
	void Set(const char* name, int32 value);
	int32 Get(const char* name) const;
	static void GetNameAndScope(const char* variable, std::string& varScope, std::string& varName);

	// Reads/writes a "<6-char scope tag><name>" variable - the wire
	// format SETGLOBAL/Global/GlobalGT/GlobalLT/IncrementGlobal all take
	// (see GetNameAndScope() above). LOCALS goes to `sender`'s own
	// per-object variables, keyed by the bare name; everything else
	// (GLOBAL, MYAREA, an explicit area code - area-scoped variables
	// aren't split into their own per-area tables yet, a known,
	// pre-existing simplification) goes to Core::Get()->Vars(), also
	// keyed by the bare name, matching the convention every other
	// GLOBAL-writing action (SG, AddGlobals, IncrementChapter, ...)
	// already uses.
	static int32 GetScoped(Object* sender, const char* prefixedName);
	static void SetScoped(Object* sender, const char* prefixedName, int32 value);

	void Print(const char* name) const;
	void PrintAll() const;

	// All (name, value) pairs, for GamResource to persist to a save file.
	std::vector<std::pair<std::string, int32>> All() const;

private:
	typedef std::map<std::string, uint32> VariablesMap;
	VariablesMap fVariables;
};

#endif /* __VARIABLES_H */
