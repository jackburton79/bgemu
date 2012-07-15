/*
 * Scriptable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTABLE_H
#define __SCRIPTABLE_H

class Scriptable;
class ScriptRoundResults {
public:
	ScriptRoundResults();


};


class Scriptable {
public:
	Scriptable();
	virtual ~Scriptable();

	void NewScriptRound();

	ScriptRoundResults* LastScriptRoundResults();
	ScriptRoundResults* CurrentScriptRoundResults();

private:
	ScriptRoundResults* fCurrentScriptRoundResults;
	ScriptRoundResults* fLastScriptRoundResults;
};

#endif // __SCRIPTABLE_H
