/*
 * RoundResults.cpp
 *
 *  Created on: 01/jan/2019
 *      Author: Stefano Ceccherini
 */


#include "RoundResults.h"

#include "Actor.h"
#include "CreResource.h"
#include "Region.h"

// ScriptResults
ScriptResults::ScriptResults()
{
	Clear();
}


void
ScriptResults::Clear()
{
	fRegionsList.clear();
	fSourcesList.clear();
	fTargetsList.clear();
}


void
ScriptResults::SetActorSaw(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::SAW, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(target->CRE()->GlobalActorEnum(), resultEntry));
}


void
ScriptResults::SetActorHeard(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::HEARD, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(target->CRE()->GlobalActorEnum(), resultEntry));
}


void
ScriptResults::SetActorAttacked(const Actor* actor, const Actor* target)
{
	result_entry resultEntry = { actor, ScriptResults::ATTACK, target };
	fSourcesList.insert(std::make_pair(actor->CRE()->GlobalActorEnum(), resultEntry));
	fTargetsList.insert(std::make_pair(target->CRE()->GlobalActorEnum(), resultEntry));
}


void
ScriptResults::SetActorEnteredRegion(const Actor* actor, const Region* region)
{
	fRegionsList.insert(std::make_pair(region->Name(), actor->CRE()->GlobalActorEnum()));
}


void
ScriptResults::SetActorClickedObject(const Actor* actor, const Object* object)
{
	result_entry resultEntry = { actor, ScriptResults::CLICKED, object };
	fClickedObjects[object->Name()] = resultEntry;
	std::cout << "SetActorClicked()" << actor->Name();
	std::cout << " " << object->Name() << std::endl;
}


Actor*
ScriptResults::GetActorWhoClickedObject(const Object* object) const
{
	std::cout << "GetActorClicked() " << object->Name() << " ?";
	objects_map::const_iterator i = fClickedObjects.find(object->Name());
	if (i != fClickedObjects.end()) {
		std::cout << ": " << i->second.actor->Name() << std::endl;
		return const_cast<Actor*>(i->second.actor);
	}
	std::cout << "no" << std::endl;
	return NULL;
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
ScriptResults::HasActorEnteredRegion(const Actor* actor, const Region* region) const
{
	std::pair<regions_map::const_iterator, regions_map::const_iterator> range;
	range = fRegionsList.equal_range(region->Name());
	for (regions_map::const_iterator i = range.first;
		i != range.second; i++) {	
		if (i->second == actor->CRE()->GlobalActorEnum())
			return true;
	}
	return false;
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
		if (i->second.action == action && i->second.actor == actor) {
			std::cout << "found: " << actor->Name() << std::endl;
			return true;
		}
	}
	
	return false;
}
