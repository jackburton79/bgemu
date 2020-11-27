/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Object.h"

#include "Action.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Core.h"
#include "Game.h"
#include "IDSResource.h"
#include "Party.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"

#include <algorithm>


// TODO: remove this dependency
#include "CreResource.h"

bool Object::sDebug = false;

trigger_entry::trigger_entry(std::string trigName)
	:
	trigger_name(trigName),
	target_id(-1)
{
}


trigger_entry::trigger_entry(std::string trigName, Object* targetObject)
	:
	trigger_name(trigName)
{
	target_id = targetObject->GlobalID();
}


// TODO: We always cast to Actor.
// Either move the methods to actor, or merge the classes
Object::Object(const char* name, object_type objectType, const char* scriptName)
	:
	Referenceable(1),
	fName(name),
	fType(objectType),
	fGlobalID(-1),
	fTicks(0),
	fTicksIdle(0),
	fVisible(true),
	fActive(false),
	fIsInterruptable(true),
	fWaitTime(0),
	fCurrentAction(NULL),
	fLastTrigger(NULL),
	fRegion(NULL),
	fToDestroy(false)
{
	if (scriptName != NULL) {
		::Script* script = Core::ExtractScript(scriptName);
		if (script != NULL)
			AddScript(script);
	}
	trigger_entry trig("ONCREATION()", this);
	AddTrigger(trig);
}


Object::~Object()
{
	for (ScriptsList::iterator i = fScripts.begin();
		i != fScripts.end(); i++) {
		delete *i;
	}
	fScripts.clear();
}


void
Object::Print() const
{
	std::cout << Name() << std::endl;
}


const char*
Object::Name() const
{
	return fName.c_str();
}


void
Object::SetName(const char* name)
{
	fName = name;
}


void
Object::SetGlobalID(uint16 id)
{
	fGlobalID = id;
	// TODO: Not really nice
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->CRE() != NULL)
		actor->CRE()->SetGlobalActorEnum(id);
}


uint16
Object::GlobalID() const
{
	// TODO: Not really nice
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor != NULL && actor->CRE() != NULL)
		return actor->CRE()->GlobalActorEnum();
		
	return fGlobalID;
}


// Returns true if an object was just instantiated
// false if it was already existing (loaded from save)
bool
Object::IsNew() const
{
	return GlobalID() == (uint16)-1;
}


int32
Object::GetVariable(const char* name) const
{
	return fVariables.Get(name);
}


void
Object::SetVariable(const char* name, int32 value)
{
	fVariables.Set(name, value);
}


void
Object::Clicked(Object* clicker)
{
	trigger_entry entry("Clicked", clicker);
	AddTrigger(entry);
}


void
Object::EnteredRegion(Region* region)
{
	fRegion = region;
}


void
Object::ExitedRegion(Region* region)
{
	fRegion = NULL;
}


bool
Object::IsVisible() const
{
	return fVisible;
}


bool
Object::IsInsideVisibleArea() const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return true;
	IE::rect rect = Core::Get()->CurrentRoom()->VisibleMapArea();
	if (rect_contains(rect, actor->Position()))
		return true;
	return false;
}


/* virtual */
void
Object::Update(bool scripts)
{
	//PrintTriggers();

	fTicks++;

	bool isArea = dynamic_cast<RoomBase*>(this) != NULL;
	Actor* actor = dynamic_cast<Actor*>(this);
	bool isActor = actor != NULL;
	bool cutscene = Core::Get()->CutsceneMode(); 	
	if (cutscene) {
		if (isArea || !isActor)
			scripts = false;
	}
	if (scripts) {
		_ExecuteScripts(8);
	}

	if (cutscene && actor != NULL && actor->InParty() && actor != Core::Get()->CutsceneActor())
		return;

	ExecuteActions();
}


void
Object::SetActive(bool active)
{
	fActive = active;
}


bool
Object::IsActive() const
{
	return fActive;
}


void
Object::AddAction(Action* action, bool now)
{
	if (now && IsActionListEmpty()) {
		//std::cout << "action was instant and we execute it now!" << std::endl;
		_ExecuteAction(*action);
	} else
		fActions.push_back(action);
}


void
Object::ExecuteActions()
{
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->IsWalking()) {
		actor->HandleWalking();
		return;
	}

	if (fCurrentAction == NULL && !fActions.empty()) {
		fCurrentAction = fActions.front();
		fActions.pop_front();
	}

	if (fCurrentAction != NULL)
		_ExecuteAction(*fCurrentAction);

	if (fCurrentAction != NULL) {
		if (fCurrentAction->Completed()) {
			//std::cout << "action " << fCurrentAction->Name() << " was completed. Removing." << std::endl;
			delete fCurrentAction;
			fCurrentAction = NULL;
		}
	}
}


bool
Object::IsActionListEmpty() const
{
	return fCurrentAction == NULL && fActions.empty();
}


void
Object::ClearActionList()
{
	for (std::list<Action*>::iterator i = fActions.begin();
									i != fActions.end(); i++) {
		delete *i;
	}
	fActions.clear();
	SetInterruptable(true);
}


void
Object::AddTrigger(trigger_entry entry)
{
	fTriggers.push_back(entry);
	if (entry.target_id != (uint16)-1)
		fLastTrigger = Core::Get()->GetObject(entry.target_id);
}


bool
Object::HasTrigger(std::string trigName) const
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		if (i->trigger_name == trigName)
			return true;
	}
	return false;
}


bool
Object::HasTrigger(std::string trigName, trigger_node* triggerNode) const
{
	object_node* objectNode = triggerNode->Object();
	if (objectNode == NULL)
		return false;
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry &entry = *i;
		if (entry.trigger_name == trigName) {
			Object* target = Core::Get()->GetObject(entry.target_id);
			Actor* actor = dynamic_cast<Actor*>(target);
			if (actor != NULL && actor->MatchNode(objectNode)) {
				std::cout << Name() << "HasTrigger " << trigName << " -> " << actor->Name() << std::endl;
				std::cout << "LastTrigger: " << fLastTrigger->Name() << std::endl;
				return true;
			}
		}
	}
	return false;
}


Object*
Object::FindTrigger(std::string trigName) const
{
	// TODO: Since we usually use this for "LastAttacker", "LastSeen", etc.
	// we start searching from the last item
	std::list<trigger_entry>::const_reverse_iterator i;
	for (i = fTriggers.rbegin(); i != fTriggers.rend(); i++) {
		if (i->trigger_name == trigName)
			return Core::Get()->GetObject(i->target_id);
	}
	return NULL;
}


Object*
Object::LastTrigger() const
{
	return fLastTrigger;
}


void
Object::PrintTriggers()
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry& entry = *i;
		Object* object = Core::Get()->GetObject(entry.target_id);
		std::cout << Name() << ": " << entry.trigger_name;
		if (object != NULL)
			std::cout << " -> " << object->Name();
		std::cout << std::endl;
	}
}


void
Object::ClearTriggers()
{
	fTriggers.clear();
	fLastTrigger = NULL;
}


void
Object::SetInterruptable(bool interrupt)
{
	fIsInterruptable = interrupt;
}


bool
Object::IsInterruptable() const
{
	return fIsInterruptable;
}


void
Object::AddScript(::Script* script)
{
	if (script != NULL)
		script->SetSender(this);
	fScripts.push_back(script);
}


void
Object::ClearScripts()
{
	for (ScriptsList::iterator i = fScripts.begin();
		i != fScripts.end(); i++) {
		delete *i;
	}
	fScripts.clear();
}


void
Object::SetWaitTime(int32 waitTime)
{
	fWaitTime += waitTime;
}


IE::point
Object::NearestPoint(const IE::point& comingFrom) const
{
	IE::point targetPoint;
	IE::rect frame = Frame();
		
	if (comingFrom.x <= frame.x_min)
		targetPoint.x = frame.x_min;
	else if (comingFrom.x >= frame.x_max)
		targetPoint.x = frame.x_max;
	if (comingFrom.y <= frame.y_min)
		targetPoint.y = frame.y_min;
	else if (comingFrom.y >= frame.y_max)
		targetPoint.y = frame.y_max;

	return targetPoint;
}


void
Object::DestroySelf()
{
	fToDestroy = true;
	std::cout << Name() << ": DestroySelf()" << std::endl;
}


bool
Object::ToBeDestroyed() const
{
	return fToDestroy;
}


/* static */
void
Object::SetDebug(bool debug)
{
	sDebug = debug;
}


/* virtual */
void
Object::LastReferenceReleased()
{
	if (sDebug) {
		std::cout << "Object (" << Name() << ")::LastReferenceReleased()";
		std::cout << std::endl;	
	}
	delete this;
}


void
Object::_ExecuteScripts(int32 maxLevel)
{
	if (fTicks % 16 != GlobalID() % 16)
		return;

	if (!IsActionListEmpty())
		return;
		
	bool runScripts = false;
	//if (fTicksIdle++ > 5)
		runScripts = true;
	
	//if (!IsInterruptable())
		//return;
	Actor* actor = dynamic_cast<Actor*>(this);
	if (!IsInsideVisibleArea()) {
		if (actor == NULL || !actor->InParty()) {
			if (fTicks % 60 != 0)
				runScripts = false;
		}
	}
		
	if (dynamic_cast<RoomBase*>(this) != NULL)
		runScripts = true;

	if (!runScripts)
		return;

	fTicksIdle = 0;
	
	if (Core::Get()->CutsceneMode())
		maxLevel = 1;

	if (!IsInterruptable())
		return;

	try {
		bool continuing = false;
		for (int32 i = 0; i < maxLevel; i++) {
			if (fScripts.at(i) != NULL) {
				if (!fScripts[i]->Execute(continuing)) {
					//std::cout << Name() << ": script " << i << " returned false." << std::endl;
					break;
				}
			}
		}
	} catch (...) {
	}
	
	if (true)
		ClearTriggers();
}


void
Object::_ExecuteAction(Action& action)
{
	std::cout << Name() << " executes " << action.Name() << std::endl;
	action();
}

