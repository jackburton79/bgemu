/*
 * ScriptContext.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTCONTEXT_H
#define __SCRIPTCONTEXT_H

#include "Script.h"

class Object;
class ScriptContext {
public:
	ScriptContext(Object* target, Script* script);
	~ScriptContext();

	void ExecuteScript();

private:
	Object* fTarget;
	Script* fScript;
	int fOrTriggers;

	bool _CheckTriggers(node* conditionNode);
	bool _EvaluateTrigger(trigger* trig);

	void _ExecuteActions(node* node);
	void _ExecuteAction(action* act);
};

#endif // __SCRIPTCONTEXT_H
