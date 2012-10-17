/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Actor.h"
#include "CreResource.h"
#include "IDSResource.h"
#include "Object.h"
#include "ResManager.h"
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
Object::IsVisible() const
{
	return fVisible;
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
		actor->UpdateMove(actor->IsFlying());
	}
}


void
Object::SetScript(Script* script)
{
	fScript = script;
	fScript->SetTarget(this);
}


static bool
MatchEA(uint8 toCheck, uint8 target)
{
	if (target == 0)
		return true;

	if (toCheck == target)
		return true;

	const char* eaString = EAIDS()->ValueFor(target);

	if (strcasecmp(eaString, "GOODCUTOFF") == 0) {
		if (toCheck <= target)
			return true;
	} else if (strcasecmp(eaString, "EVILCUTOFF") == 0) {
		if (toCheck >= target)
			return true;
	}
	return false;
}


bool
Object::MatchNode(object_node* node)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if ((node->name[0] == '\0' || !strcasecmp(node->name, actor->Name()))
		&& (node->classs == 0 || node->classs == cre->Class())
		&& (node->race == 0 || node->race == cre->Race())
		&& (node->alignment == 0 || node->alignment == cre->Alignment())
		&& (node->gender == 0 || node->gender == cre->Gender())
		&& (node->general == 0 || node->general == cre->General())
		&& (node->specific == 0 || node->specific == cre->Specific())
		&& MatchEA(node->ea, cre->EnemyAlly()))
		return true;
	return false;
}


bool
Object::Match(Object* objectA, Object* objectB)
{
	Actor* actorA = dynamic_cast<Actor*>(objectA);
	Actor* actorB = dynamic_cast<Actor*>(objectB);
	if (actorA == NULL || actorB == NULL)
		return false;

	CREResource* creA = actorA->CRE();
	CREResource* creB = actorB->CRE();

	if (strcmp(actorA->Name(), actorB->Name()) == 0
		&& (creA->Class() == creB->Class())
		&& (creA->Race() == creB->Race())
		&& (creA->Alignment() == creB->Alignment())
		&& (creA->Gender() == creB->Gender())
		&& (creA->General() == creB->General())
		&& (creA->Specific() == creB->Specific())
		&& MatchEA(creA->EnemyAlly(), creB->EnemyAlly()))
		return true;
	return false;
}
