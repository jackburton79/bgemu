/*
 * Scriptable.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Object.h"

#include <algorithm>

Object::Object(const char* name)
	:
	fName(name),
	fVisible(true),
	fCurrentScriptRoundResults(NULL),
	fLastScriptRoundResults(NULL)
{
}


Object::~Object()
{
	delete fCurrentScriptRoundResults;
	delete fLastScriptRoundResults;
}


const char*
Object::Name() const
{
	return fName;
}


bool
Object::See(Object* object)
{
	if (object == NULL)
		return false;
	return object->IsVisible();
}


bool
Object::IsVisible() const
{
	return fVisible;
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
