/*
 * RoundResults.cpp
 *
 *  Created on: 01/jan/2019
 *      Author: Stefano Ceccherini
 */


#include "RoundResults.h"

#include "Actor.h"
#include "CreResource.h"


// ScriptResults
ScriptResults::ScriptResults()
{
	Clear();
}


void
ScriptResults::Clear()
{
	fSourcesList.clear();
	fTargetsList.clear();
}


void
ScriptResults::ActorSaw(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::SAW, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
}


void
ScriptResults::ActorHeard(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::HEARD, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
}


void
ScriptResults::ActorAttacked(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::ATTACK, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
}


bool
ScriptResults::WasActorAttackedBy(const Actor* actor, object_node* node) const
{
	return _FindActionByTargetObject(actor, ATTACK, node);
}


bool
ScriptResults::WasActorSeenBy(const Actor* actor, object_node* node) const
{
	return _FindActionByTargetObject(actor, SAW, node);
}


bool
ScriptResults::WasActorSeenBy(const Actor* target, const Actor* observer) const
{
	return _FindActionByTargetActor(target, SAW, observer);
}


bool
ScriptResults::WasActorHeardBy(const Actor* actor, object_node* node) const
{
	return _FindActionByTargetObject(actor, HEARD, node);
}


bool
ScriptResults::_FindActionByTargetObject(const Actor* target, int action, object_node* node) const
{
	std::pair <results_map::const_iterator, results_map::const_iterator> range;
	range = fTargetsList.equal_range(target->CRE()->GlobalActorEnum());
	for (results_map::const_iterator i = range.first;
		i != range.second; i++) {
		if (i->second.action == action && i->second.actor->MatchNode(node))
			return true;
	}
	return false;
}


bool
ScriptResults::_FindActionByTargetActor(const Actor* target, int action, const Actor* actor) const
{
	std::pair <results_map::const_iterator, results_map::const_iterator> range;
	range = fTargetsList.equal_range(target->CRE()->GlobalActorEnum());
	for (results_map::const_iterator i = range.first;
		i != range.second; i++) {
		if (i->second.action == action && i->second.actor == actor)
			return true;
	}
	
	return false;
}
