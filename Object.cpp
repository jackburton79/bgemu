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

	// TODO: Write code to be able to check if an item is inside
	// a given group: I.E: is an item a "GENERAL_ITEM"
	CREResource* cre = actor->CRE();
	//printf("tested actor general: %d\n", cre->General());
	if (IsName(node->name)
		&& (IsClass(node->classs))
		&& (IsRace(node->race))
		&& (IsAlignment(node->alignment))
		&& (IsGender(node->gender))
		&& (IsGeneral(node->general))
		&& (IsSpecific(node->specific))
		&& IsEnemyAlly(node->ea))
		return true;
	return false;
}


/* static */
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


bool
Object::IsName(const char* name)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;
	if (name[0] == '\0' || !strcasecmp(name, actor->Name()))
		return true;
	return false;
}


bool
Object::IsClass(int c)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (c == 0 || c == cre->Class())
		return true;

	return false;
}


bool
Object::IsRace(int race)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (race == 0 || race == cre->Race())
		return true;

	return false;
}


bool
Object::IsGender(int gender)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (gender == 0 || gender == cre->Gender())
		return true;

	return false;
}


bool
Object::IsGeneral(int general)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	// TODO: No idea if it's correct or not
	const char* stringValue = GeneralIDS()->ValueFor(general);
	//if (!stringValue)
	CREResource* cre = actor->CRE();
	if (general == 0 || general == cre->General())
		return true;

	return false;
}


bool
Object::IsSpecific(int specific)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (specific == 0 || specific == cre->Specific())
		return true;

	return false;
}


bool
Object::IsAlignment(int alignment)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (alignment == 0 || alignment == cre->Alignment())
		return true;

	return false;
}


bool
Object::IsEnemyAlly(int ea)
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (MatchEA(ea, cre->EnemyAlly()))
		return true;

	return false;
}
