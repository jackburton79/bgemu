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

	bool IsName(const char* name);
	bool IsClass(int c);
	bool IsRace(int race);
	bool IsGender(int gender);
	bool IsGeneral(int general);
	bool IsSpecific(int specific);
	bool IsAlignment(int alignment);
	bool IsEnemyAlly(int ea);


protected:
	const char* fName;
	bool fVisible;

	Script* fScript;

	uint16 fTicks;
};

#endif // __SCRIPTABLE_H
