/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Object.h"

#include "Action.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Core.h"
#include "Game.h"
#include "IDSResource.h"
#include "Log.h"
#include "Party.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"
#include "SpellEffect.h"

#include <algorithm>
#include <assert.h>

// TODO: remove this dependency
#include "CreResource.h"

bool Object::sDebug = false;


Outline::Outline(const ::Polygon& polygon, GFX::Color color)
	:
	fColor(color),
	fType(OUTLINE_POLY),
	fRect(gfx_rect_to_rect(polygon.Frame())),
	fPolygon(polygon)
{
}


Outline::Outline(const IE::rect& rect, GFX::Color color)
	:
	fColor(color),
	fType(OUTLINE_RECT),
	fRect(rect)
{
}


GFX::Color
Outline::Color() const
{
	return fColor;
}


int
Outline::Type() const
{
	return fType;
}


::Polygon
Outline::Polygon() const
{
	return fPolygon;
}


IE::rect
Outline::Rect() const
{
	return fRect;
}


// trigger_entry
trigger_entry::trigger_entry(const std::string& trigName)
	:
	trigger_name(trigName),
	target_id(-1)
{
}


trigger_entry::trigger_entry(const std::string& trigName, Object* targetObject)
	:
	trigger_name(trigName),
	target_id(targetObject->GlobalID())
{
}


// TODO: We cast to Actor very often in various methods.
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
	fActive(true),
	fIsInterruptable(true),
	fWaitTime(0),
	fCurrentAction(NULL),
	fLastTrigger(NULL),
	fArea(NULL),
	fRegion(NULL),
	fToDestroy(false)
{
	if (scriptName != NULL) {
		::Script* script = Core::ExtractScript(scriptName);
		if (script != NULL)
			AddScript(script);
	}
	trigger_entry trig("OnCreation", this);
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


Object::object_type
Object::Type() const
{
	return fType;
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
	return fGlobalID;
}


AreaRoom*
Object::Area() const
{
	return fArea;
}


void
Object::SetArea(AreaRoom* area)
{
	fArea = area;

	if (area == NULL)
		return;

	// TODO: Not really nice
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->CRE() != NULL) {
		if (actor->CRE()->PermanentStatus() == 2048) // STATE_DEAD
			actor->SetAnimationAction(ACT_DEAD);
		else
			actor->SetAnimationAction(ACT_STANDING);
	}
}


// Returns true if an object was just instantiated
// false if it was already existing (loaded from save)
bool
Object::IsNew() const
{
	return GlobalID() == (uint16)-1;
}


/* virtual */
::Outline
Object::Outline() const
{
	GFX::Color color = { 0, 0, 0 };
	return ::Outline(Frame(), color);
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
	IE::rect rect = Area()->VisibleMapArea();
	if (rect_contains(rect, actor->Position()))
		return true;
	return false;
}


/* virtual */
void
Object::Update(bool scripts)
{
	fTicks++;

	if (sDebug)
		std::cout << Name() << ": Update(): ticks = " << std::dec << fTicks << std::endl;

	bool cutscene = Core::Get()->CutsceneMode(); 	
	if (cutscene)
		scripts = false;

	if (scripts) {
		_HandleScripting(8);
	}

	ExecuteActions();

	_ApplySpellEffects();
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
Object::AddAction(Action* action)
{
	SetActive(true);
	if (action->IsInstant() && IsActionListEmpty()) {
		//std::cout << "action was instant and we execute it now!" << std::endl;
		fCurrentAction = action;
		_ExecuteAction(*action);
		return;
	}

	fActions.push_back(action);
}


void
Object::ExecuteActions()
{
#if 0
	if (fActions.size() > 0) {
		std::cout << Name() << " action list:" << std::endl;
		// dump action list
		for (std::list<Action*>::iterator i = fActions.begin();
										i != fActions.end(); i++) {

			std::cout << (*i)->Name() << std::endl;
		}
	}
	if (fCurrentAction != NULL)
		std::cout << "current action: " << fCurrentAction->Name() << std::endl;
#endif
	// TODO: handle uninterruptable action

	 while (true) {
		if (fCurrentAction == NULL)
			PopNextAction();

		if (fCurrentAction == NULL)
			break;

		_ExecuteAction(*fCurrentAction);

		// fCurrentAction is not completed, will
		// do another execution next time
		if (fCurrentAction != NULL)
			break;
	}

	//std::cout << Name() << " executed " << count << " actions" << std::endl;
}


bool
Object::IsActionListEmpty() const
{
	return fCurrentAction == NULL && fActions.empty();
}


Action*
Object::PopNextAction()
{
	if (!fActions.empty()) {
		fCurrentAction = fActions.front();
		fActions.pop_front();
	}
	return fCurrentAction;
}


void
Object::ClearCurrentAction()
{
	if (fCurrentAction != NULL) {
		delete fCurrentAction;
		fCurrentAction = NULL;
	}
	SetInterruptable(true);
}


void
Object::ClearActionList()
{
	ClearCurrentAction();
	for (std::list<Action*>::iterator i = fActions.begin();
									i != fActions.end(); i++) {
		delete *i;
	}
	fActions.clear();
}


void
Object::AddTrigger(const trigger_entry& entry)
{
	fTriggers.push_back(entry);
	if (entry.target_id != (uint16)-1)
		fLastTrigger = Area()->GetObject(entry.target_id);
}


bool
Object::HasTrigger(const std::string& trigName) const
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		if (i->trigger_name == trigName)
			return true;
	}
	return false;
}


bool
Object::HasTrigger(const std::string& trigName, trigger_params* triggerNode) const
{
	object_params* objectNode = triggerNode->Object();
	if (objectNode == NULL)
		return false;
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry &entry = *i;
		if (entry.trigger_name == trigName) {
			Object* target = Area()->GetObject(entry.target_id);
			Actor* actor = dynamic_cast<Actor*>(target);
			if (actor != NULL && actor->MatchNode(objectNode)) {
				std::cout << Name() << " HasTrigger " << trigName << " -> " << actor->Name() << std::endl;
				std::cout << "LastTrigger: " << fLastTrigger->Name() << std::endl;
				return true;
			}
		}
	}
	return false;
}


Object*
Object::FindTrigger(const std::string& trigName) const
{
	// TODO: Since we usually use this for "LastAttacker", "LastSeen", etc.
	// we start searching from the last item
	std::list<trigger_entry>::const_reverse_iterator i;
	for (i = fTriggers.rbegin(); i != fTriggers.rend(); i++) {
		if (i->trigger_name == trigName)
			return ((AreaRoom*)Core::Get()->CurrentRoom())->GetObject(i->target_id);
	}
	return NULL;
}


Object*
Object::LastTrigger() const
{
	return fLastTrigger;
}


void
Object::PrintTriggers() const
{
	std::list<trigger_entry>::const_iterator i;
	for (i = fTriggers.begin(); i != fTriggers.end(); i++) {
		const trigger_entry& entry = *i;
		Object* object = Area()->GetObject(entry.target_id);
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
Object::AddSpellEffect(SpellEffect* effect)
{
	fSpellEffects.push_back(effect);
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
	delete this;
}


void
Object::_HandleScripting(int32 maxLevel)
{
	if (sDebug)
		std::cout << Name() << ": _HandleScripting()" << std::endl;

	if (fTicks % 16 != GlobalID() % 16)
		return;

	bool runScripts = (fTicksIdle > 15) || IsActionListEmpty();
	
	Actor* actor = dynamic_cast<Actor*>(this);
	if (!IsInsideVisibleArea()) {
		if (actor == NULL || !actor->InParty()) {
			if (fTicks % 60 != 0)
				runScripts = false;
		}
	}

	if (!runScripts) {
		fTicksIdle++;
		return;
	}
	
	/*if (Core::Get()->CutsceneMode())
		maxLevel = 1;
*/
	if (sDebug) {
		std::cout << Name() << ": _ExecuteScripts(): run scripts (ticks=" << fTicks;
		std::cout << ", globalID=" << GlobalID() << ")" << std::endl;
	}

	fTicksIdle = 0;
	_ExecuteScripts(maxLevel);

	if (true)
		ClearTriggers();
}


void
Object::_ExecuteScripts(int32 maxLevel)
{
	if (!IsInterruptable())
		return;

	maxLevel = std::min((size_t) (maxLevel), fScripts.size());
	try {
		bool continuing = false;
		bool actionDone = false;
		for (int32 i = 0; i < maxLevel; i++) {
#if 0
			std::cout << Name() << ": script " << i << std::endl;
#endif
			if (fScripts.at(i) == NULL)
				continue;

			fScripts[i]->Execute(continuing, actionDone);
			if (actionDone) {
				//std::cout << Name() << ": script " << i << " returned false." << std::endl;
				break;
			}
		}
	} catch (std::exception& e) {
		std::cerr << Log::Red << e.what() << std::endl;
	} catch (...) {
		std::cerr << Log::Red << "Exception while running script!" << std::endl;
	}
}


void
Object::_ExecuteAction(Action& action)
{
	SetInterruptable(false);
	//std::cout << Name() << " executes " << action.Name() << std::endl;
	action();

	// if completed, clear
	if (fCurrentAction != NULL && fCurrentAction->Completed()) {
		//std::cout << "action " << fCurrentAction->Name() << " was completed. Removing." << std::endl;
		ClearCurrentAction();
	}
}


void
Object::_ApplySpellEffects()
{
	for (std::list<SpellEffect*>::iterator i = fSpellEffects.begin();
			i != fSpellEffects.end(); i++) {
		/*if ((*i)->Name() == "WIZARD_DISINTEGRATE2_IGNORE_RESISTANCE") {
			DestroySelf();
			break;
		}*/
	}
}
