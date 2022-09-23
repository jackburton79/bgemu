/*
 * Triggers.h
 *
 *  Created on: 16 nov 2020
 *      Author: Stefano Ceccherini
 */

#ifndef GAMESCRIPT_TRIGGERS_H_
#define GAMESCRIPT_TRIGGERS_H_

#include "IETypes.h"

#include <string>

struct Triggers {
	int32 id;
	const char* name;
};


std::string GetTriggerName(int32 id);
int32 GetTriggerID(std::string name);

#endif /* GAMESCRIPT_TRIGGERS_H_ */
