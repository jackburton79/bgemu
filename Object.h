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

struct action_node;
struct object_node;
class Script;
class Object {
public:
	Object(const char* name);
	virtual ~Object();

	const char* Name() const;

	bool AddAction(action_node* act);
	bool IsActionListEmpty() const;

	bool See(Object* object);
	bool IsVisible() const;

	Object* LastAttacker() const;

	uint16 EnemyAlly() const;

	void Update();

	void SetScript(Script* script);

	bool MatchNode(object_node* node);
	static bool Match(Object* a, Object* b);

protected:
	const char* fName;
	bool fVisible;

	Script* fScript;

	std::list<action_node*> fActions;

	uint16 fTicks;
};

#endif // __SCRIPTABLE_H
