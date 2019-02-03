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


IE::point
Object::Position() const
{
	IE::point point = { -1, -1 };
	return point;
}


Variables&
Object::Vars()
{
	return fVariables;
}


void
Object::Clicked(Object* clicker)
{
	//fCurrentScriptRoundResults->fClicker = clicker;
}


void
Object::ClickedOn(Object* target)
{
	if (target)
		target->Clicked(this);
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
	IE::rect rect = Core::Get()->CurrentRoom()->VisibleMapArea();
	if (rect_contains(rect, Position()))
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
Object::AddAction(Action* action)
{
	fActions.push_back(action);
}


void
Object::ExecuteActions()
{
	if (fWaitTime > 0) {
		//std::cout << Name() << " : wait time " << fWaitTime << std::endl;
		fWaitTime--;
		return;
	}

	if (fActions.size() != 0) {
		std::list<Action*>::iterator i = fActions.begin();
		while (i != fActions.end()) {
			Action& action = **i;
			if (action.Completed()) {
				//std::cout << Name() << " action completed" << std::endl;				
				delete *i;
				i = fActions.erase(i);
			} else {
				//std::cout << Name() << " execute action" << std::endl;
				action();
				break;
			}
		}
	}
}


bool
Object::IsActionListEmpty() const
{
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
	fWaitTime = waitTime;
}


/* virtual */
IE::point
Object::NearestPoint(const IE::point& point) const
{
	return Position();
}


void
Object::DestroySelf()
{
	fToDestroy = true;
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
	bool runScripts = false;
	if ((fTicks++ % 15 == 0) || IsActionListEmpty())
		runScripts = true;
	
	if (!IsInterruptable())
		return;
	
	if (dynamic_cast<RoomBase*>(this) == NULL
		&& dynamic_cast<Region*>(this) == NULL
		&& !IsInsideVisibleArea())
		return;

	if (!runScripts)
		return;
		
	try {
		bool continuing = false;
		for (int32 i = 0; i < maxLevel; i++) {		
			if (fScripts.at(i) != NULL) {
				if (!fScripts[i]->Execute(continuing))
					break;
			}
		}
	} catch (...) {
	}
}


