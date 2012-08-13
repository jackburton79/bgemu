/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Actor.h"
#include "CreResource.h"
#include "Object.h"
#include "Script.h"

#include <algorithm>

Object::Object(const char* name)
	:
	fName(name),
	fVisible(true),
	fScript(NULL),
	fTicks(0)
{
}


Object::~Object()
{

}


const char*
Object::Name() const
{
	return fName;
}


bool
Object::IsActionListEmpty() const
{
	return fActions.size() == 0;
}


bool
Object::AddAction(action_node* act)
{
	try {
		fActions.push_back(act);
	} catch (...) {
		return false;
	}

	return true;
}


bool
Object::IsVisible() const
{
	return fVisible;
}


Object*
Object::LastAttacker() const
{
	return NULL;
}


uint16
Object::EnemyAlly() const
{
	return 0;
}


void
Object::Update()
{
	if (++fTicks == 15) {
		fTicks = 0;
		if (fScript != NULL)
			fScript->Execute();
	}
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL) {
		actor->UpdateMove();
	}
}


void
Object::SetScript(Script* script)
{
	fScript = script;
	fScript->SetTarget(this);
}


bool
Object::MatchNode(object_node* node)
{
	// TODO:
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;
	CREResource* cre = actor->CRE();
	if ((node->name[0] == '\0' || !strcmp(node->name, actor->Name()))
		&& (node->classs == 0 || node->classs == cre->Class())
		&& (node->race == 0 || node->alignment == cre->Race())
		&& (node->gender == 0 || node->gender == cre->Gender())
		&& (node->general == 0 || node->general == cre->General())
		&& (node->specific == 0 || node->specific == cre->Specific()))
		return true;
	return false;
}


bool
Object::Match(Object* a, Object* b)
{

	return false;
		/*
				&& node->classs == Class()
				&& node->alignment == Alignment()
				&& node->race == Race()
				&& node->gender == Gender()
				&& node->type == Type();*/
}
