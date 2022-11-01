/*
 * Actions.h
 *
 *  Created on: 12 sep 2022
 *      Author: Stefano Ceccherini
 */

#ifndef GAMESCRIPT_ACTIONS_H_
#define GAMESCRIPT_ACTIONS_H_

#include "IETypes.h"

#include <string>

struct Actions {
	int32 id;
	const char* name;
};


std::string GetActionName(int32 id);
int32 GetActionID(std::string name);

#endif /* GAMESCRIPT_ACTIONS_H_ */
