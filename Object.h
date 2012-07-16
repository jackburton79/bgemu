/*
 * Scriptable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTABLE_H
#define __SCRIPTABLE_H

#include "SupportDefs.h"

class Object;
class ScriptRoundResults {
public:
	ScriptRoundResults();

};


class Object {
public:
	Object(const char* name);
	virtual ~Object();

	const char* Name() const;

	bool See(Object* object);
	bool IsVisible() const;

	uint16 EnemyAlly();

	void NewScriptRound();

	ScriptRoundResults* LastScriptRoundResults();
	ScriptRoundResults* CurrentScriptRoundResults();

private:
	const char* fName;
	bool fVisible;
	ScriptRoundResults* fCurrentScriptRoundResults;
	ScriptRoundResults* fLastScriptRoundResults;
	//std::list<Object*> fAttackers;
};

#endif // __SCRIPTABLE_H
