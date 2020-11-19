/*
 * Triggers.h
 *
 *  Created on: 16 nov 2020
 *      Author: Stefano Ceccherini
 */

#ifndef TRIGGERS_H_
#define TRIGGERS_H_

#include "IETypes.h"

#include <string>

struct Triggers {
	int32 id;
	const char* name;
};


std::string GetTriggerName(int32 id);
int32 GetTriggerID(std::string name);

#endif /* TRIGGERS_H_ */
