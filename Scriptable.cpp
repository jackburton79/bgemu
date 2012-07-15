/*
 * Scriptable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Scriptable.h"

#include <algorithm>

Scriptable::Scriptable()
	:
	fCurrentScriptRoundResults(NULL),
	fLastScriptRoundResults(NULL)
{
}


Scriptable::~Scriptable()
{
	delete fCurrentScriptRoundResults;
	delete fLastScriptRoundResults;
}


ScriptRoundResults*
Scriptable::CurrentScriptRoundResults()
{
	return fCurrentScriptRoundResults;
}


ScriptRoundResults*
Scriptable::LastScriptRoundResults()
{
	return fLastScriptRoundResults;
}


void
Scriptable::NewScriptRound()
{
	delete fLastScriptRoundResults;
	std::swap(fCurrentScriptRoundResults, fLastScriptRoundResults);
	fCurrentScriptRoundResults = new ScriptRoundResults;
}


// ScriptRoundResults
ScriptRoundResults::ScriptRoundResults()
{

}
