/*
 * Scriptable.h
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#ifndef __SCRIPTABLE_H
#define __SCRIPTABLE_H

#include "SupportDefs.h"

#include <list>

struct object_node;
class Script;
class Object {
public:
	Object(const char* name);
	virtual ~Object();

	const char* Name() const;

	bool See(Object* object);
	bool IsVisible() const;

	void Update();

	void SetScript(Script* script);

	bool MatchNode(object_node* node);
	static bool Match(Object* a, Object* b);

protected:
	const char* fName;
	bool fVisible;

	Script* fScript;

	uint16 fTicks;
};

#endif // __SCRIPTABLE_H
