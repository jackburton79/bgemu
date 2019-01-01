/*
 * RoundResults.h
 *
 *  Created on: 01/jan/2019
 *      Author: Stefano Ceccherini
 */

#ifndef ROUNDRESULT_H
#define ROUNDRESULT_H

#include "IETypes.h"
#include "Reference.h"
#include "Referenceable.h"
#include "SupportDefs.h"

#include <map>
#include <string>
#include <vector>

struct object_node;
class Actor;
class Object;

struct result_entry {
	const Actor* actor;
	int action;
	const Actor* target;
};

class ScriptResults {
public:
	enum Action {
		ATTACK = 0,
		SAW = 1,
		HEARD = 2
	};
	
	ScriptResults();
	void Clear();
	
	void ActorSaw(const Actor* actor, const Actor* target);
	void ActorHeard(const Actor* actor, const Actor* target);
	void ActorAttacked(const Actor* actor, const Actor* target);
	
	bool WasActorAttackedBy(const Actor* actor, object_node* node) const;
	bool WasActorSeenBy(const Actor* actor, object_node* node) const;
	bool WasActorSeenBy(const Actor* actor, const Actor* object) const;
	bool WasActorHeardBy(const Actor* actor, object_node* node) const;
	
private:
	bool _FindActionByTargetObject(const Actor* target, int action, object_node* node) const;
	bool _FindActionByTargetActor(const Actor* target, int action, const Actor* actor) const;
	bool _FindActionBySourceObject(const Actor* source, int action, object_node* node) const;
	
	typedef std::multimap<uint16, result_entry> results_map;
	
	results_map fSourcesList;
	results_map fTargetsList;
};


#endif // ROUNDRESULT_H
