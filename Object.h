/*
 * Scriptable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTABLE_H
#define __SCRIPTABLE_H

class Object;
class ScriptRoundResults {
public:
	ScriptRoundResults();
};


class Object {
public:
	Object();
	virtual ~Object();

	void NewScriptRound();

	ScriptRoundResults* LastScriptRoundResults();
	ScriptRoundResults* CurrentScriptRoundResults();

private:
	ScriptRoundResults* fCurrentScriptRoundResults;
	ScriptRoundResults* fLastScriptRoundResults;
};

#endif // __SCRIPTABLE_H
