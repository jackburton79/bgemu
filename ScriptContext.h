/*
 * ScriptContext.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTCONTEXT_H
#define __SCRIPTCONTEXT_H

#include "Script.h"

class Scriptable;
class ScriptContext {
public:
	ScriptContext(Scriptable* target, Script* script);
	~ScriptContext();

	void ExecuteScript();

private:
	Scriptable* fTarget;
	Script* fScript;
	int fOrTriggers;

	bool _CheckTriggers(node* conditionNode);
	bool _EvaluateTrigger(trigger* trig);

	void _ExecuteActions(node* node);
	void _ExecuteAction(action* act);
};

#endif // __SCRIPTCONTEXT_H
