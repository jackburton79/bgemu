/*
 * Scriptable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Object.h"

#include <algorithm>

Object::Object()
	:
	fCurrentScriptRoundResults(NULL),
	fLastScriptRoundResults(NULL)
{
}


Object::~Object()
{
	delete fCurrentScriptRoundResults;
	delete fLastScriptRoundResults;
}


ScriptRoundResults*
Object::CurrentScriptRoundResults()
{
	return fCurrentScriptRoundResults;
}


ScriptRoundResults*
Object::LastScriptRoundResults()
{
	return fLastScriptRoundResults;
}


void
Object::NewScriptRound()
{
	delete fLastScriptRoundResults;
	std::swap(fCurrentScriptRoundResults, fLastScriptRoundResults);
	fCurrentScriptRoundResults = new ScriptRoundResults;
}


// ScriptRoundResults
ScriptRoundResults::ScriptRoundResults()
{

}
