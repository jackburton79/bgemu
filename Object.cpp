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
	name(trigName)
{
}


trigger_entry::trigger_entry(std::string trigName, Object* targetObject)
	:
	name(trigName)
{
	target = targetObject->Name();
}


// TODO: We always cast to Actor.
// Either move the methods to actor, or merge the classes
Object::Object(const char* name, const char* scriptName)
	:
	Referenceable(1),
	fName(name),
	//fGlobalEnum(0),
	fTicks(0),
	fTicksIdle(0),
	fVisible(true),
	fActive(false),
	fIsInterruptable(true),
	fWaitTime(0),
	fCurrentAction(NULL),
	fLastAttacker(-1),
	fLastClicker(-1),
	fRegion(NULL),
	fToDestroy(false)
{
	if (scriptName != NULL) {
		::Script* script = Core::ExtractScript(scriptName);
		if (script != NULL)
			AddScript(script);
	}
	trigger_entry trig("ONCREATION()");
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
		
	//std::cout << "OBJECT(" << Name() << "): NO GLOBAL ID!!!" << std::endl;
	return (uint16)-1;
}


Variables&
Object::Vars()
{
	return fVariables;
}


void
Object::Clicked(Object* clicker)
{
	fLastClicker = clicker->GlobalID();
}

Object*
Object::LastClicker() const
{
	return Core::Get()->GetObject(fLastClicker);
}


void
Object::EnteredRegion(Region* region)
{
	fRegion = region;
	//fRegion->CurrentScriptRoundResults()->SetEnteredActor(this);
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
	fTicks++;

	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL)
		actor->UpdateSee();

	bool isArea = dynamic_cast<RoomBase*>(this) != NULL;
	bool cutscene = Core::Get()->CutsceneMode(); 	
	if (isArea && cutscene)
		scripts = false;

	if (scripts) {
		_ExecuteScripts(8);		
	}

	if (cutscene && actor != NULL && Game::Get()->Party()->HasActor(actor))
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
	if (fCurrentAction != NULL) {
		if (fCurrentAction->Completed()) {
			std::cout << "action " << fCurrentAction->Name() << " was completed. Removing." << std::endl;
			delete fCurrentAction;
			fCurrentAction = NULL;
		}
	}

	if (fCurrentAction == NULL && !fActions.empty()) {
		fCurrentAction = fActions.front();
		fActions.pop_front();
	}

	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->IsWalking()) {
		actor->HandleWalking();
		return;
	}

	if (fCurrentAction != NULL)
		(*fCurrentAction)();
}


bool
Object::IsActionListEmpty() const
{
	/*std::cout << Name() << ": ";
	std::cout << "IsActionListEmpty() ? size = " << std::dec;
	std::cout << fActions.size() << std::endl;
	for (std::list<Action*>::const_iterator i = fActions.begin();
									i != fActions.end(); i++) {
		std::cout << (*i)->Name() << std::endl;
	}*/

	return fCurrentAction == NULL && fActions.size() == 0;
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
}


bool
Object::HasTrigger(std::string trigName) const
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		if (i->name == trigName)
			return true;
	}
	return false;
}


bool
Object::HasTrigger(std::string trigName, trigger_node* triggerNode)
{
	std::cout << Name() << ": HasTrigger()" << trigName << std::endl;
	object_node* objectNode = Script::FindObjectNode(this, triggerNode);
	if (objectNode == NULL)
		return false;
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry &entry = *i;
		if (entry.name == trigName) {
			// TODO: We should store more than the target name, since 
			// it's not sufficient in many cases
			Object* target = Core::Get()->GetObject(entry.target.c_str());
			Actor* actor = dynamic_cast<Actor*>(target);
			std::cout << target << std::endl;
			if (actor != NULL && actor->MatchNode(objectNode))
				return true;
		}
	}
	return false;
}


void
Object::PrintTriggers()
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry& entry = *i;		
		std::cout << entry.name << " -> " << entry.target << std::endl;
	}
}


void
Object::ClearTriggers()
{
	fTriggers.clear();
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
	}
	fScripts.clear();
}


void
Object::SetWaitTime(int32 waitTime)
{
	fWaitTime += waitTime;
}


IE::point
Object::NearestPoint(const IE::point& point) const
{
	IE::point targetPoint;
	IE::rect frame = Frame();
		
	if (point.x <= frame.x_min)
		targetPoint.x = frame.x_min;
	else if (point.x >= frame.x_max)
		targetPoint.x = frame.x_max;
	if (point.y <= frame.y_min)
		targetPoint.y = frame.y_min;
	else if (point.y >= frame.y_max)
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
	if (fTicksIdle++ > 15)
		runScripts = true;
	
	//if (!IsInterruptable())
		//return;
	if (!IsInsideVisibleArea()) {
	
#if 0
		runScripts = false;		
#else
		if (fTicks % 60 != 0)
			runScripts = false;
#endif	
	}
		
	if (dynamic_cast<RoomBase*>(this) != NULL)
		runScripts = true;

	if (!runScripts)
		return;

	fTicksIdle = 0;
	
	if (Core::Get()->CutsceneMode())
		maxLevel = 1;

	try {
		bool continuing = false;
		for (int32 i = 0; i < maxLevel; i++) {		
			if (fScripts.at(i) != NULL) {
				if (!fScripts[i]->Execute(continuing)) {
					std::cout << Name() << ": script " << i << " returned false." << std::endl;
					break;
				}
			}
		}
	} catch (...) {
	}
	
	//PrintTriggers();

	if (true)
		ClearTriggers();
}


void
Object::_ExecuteAction(Action& action)
{
	std::cout << Name() << " executes " << action.Name() << std::endl;
	action();
}

