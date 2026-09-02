/*
 * Actions.h
 *
 *  Created on: 12 sep 2022
 *      Author: Stefano Ceccherini
 */

#pragma once

#include "ActionState.h"
#include "IETypes.h"

#include <string>

class Object;
struct action_params;

// A native action implementation: reads/writes `params` and `state`,
// setting state.completed = true once done. Called once per tick for as
// long as the action is the sender's current action.
typedef void (*ActionRunFunc)(Object* sender, action_params* params, action_state& state);

struct ActionDescriptor {
	int32 id;
	const char* name;

	// NULL until this action id has been migrated to the new run-function
	// based dispatch; Object::_ExecuteAction() falls back to the legacy
	// Action-subclass path (RunLegacyAction(), below) whenever this is NULL.
	ActionRunFunc run;
};

std::string GetActionName(int32 id);
int32 GetActionID(std::string name);

// O(1) lookup used by the dispatch path in Object::_ExecuteAction().
// Returns NULL if id is not a known action.
const ActionDescriptor* GetActionDescriptor(int32 id);

// Whether action id is flagged "instant" in the INSTANT.IDS resource
// (result cached per id - see IsInstant()'s original implementation for why).
bool IsInstantAction(int32 id);

