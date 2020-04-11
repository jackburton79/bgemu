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
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"

#include <algorithm>

// TODO: We always cast to Actor.
// Either move the methods to actor, or merge the classes
Object::Object(const char* name, const char* scriptName)
	:
	Referenceable(1),
	fName(name),
	fTicks(0),
	fVisible(true),
	fActive(false),
	fIsInterruptable(true),
	fWaitTime(0),
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


uint16
Object::GlobalID() const
{
	// TODO: 
	return 0;
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
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL)
		actor->UpdateSee();
	
	bool isArea = dynamic_cast<RoomBase*>(this) != NULL;
	if (isArea && Core::Get()->CutsceneMode())
		scripts = false;
		
	if (scripts) {
		_ExecuteScripts(8);		
	}
	
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
	/*if (fWaitTime > 0) {
		std::cout << Name() << " : wait time " << std::dec << fWaitTime << std::endl;
		fWaitTime--;
		return;
	}*/

	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->IsWalking()) {
		actor->HandleWalking();
		return;
	}

	std::list<Action*>::iterator i;
	for (i = fActions.begin(); i != fActions.end();) {
		Action& action = **i;
		if (action.Completed()) {
			std::cout << "action " << action.Name() << " was completed. Removing." << std::endl;
			delete *i;
			i = fActions.erase(i);
		} else {
			//std::cout << Name() << " executing " << action.Name() << std::endl;
			action();
			break;
		}
	}
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
	return fActions.size() == 0;
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


/* virtual */
void
Object::LastReferenceReleased()
{
	delete this;
}


void
Object::_ExecuteScripts(int32 maxLevel)
{
	if (!IsActionListEmpty())
		return;
		
	bool runScripts = false;
	if ((fTicks++ % 15 == 0))
		runScripts = true;
	
	//if (!IsInterruptable())
		//return;
	
	if (!IsInsideVisibleArea()) {
		if (fTicks % 60 != 0)
			runScripts = false;
	}
		
	if (dynamic_cast<RoomBase*>(this) != NULL)
		//|| dynamic_cast<Region*>(this) != NULL)
		runScripts = true;

	if (!runScripts)
		return;
		
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
}


void
Object::_ExecuteAction(Action& action)
{
	std::cout << Name() << " executes " << action.Name() << std::endl;
	action();
}

