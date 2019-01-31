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
	fWaitTime(0),
	fVisible(true),
	fRegion(NULL),
	fStale(false)
{
	if (scriptName != NULL) {
		BCSResource* scriptResource = gResManager->GetBCS(scriptName);
		if (scriptResource != NULL) {
			::Script* script = scriptResource->GetScript();
			if (script != NULL)
				AddScript(script);
		}
		gResManager->ReleaseResource(scriptResource);
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
	if (scripts) {
		// TODO: Make Object::Update() virtual and override
		// in subclasses to avoid dynamic casting
		Actor* actor = dynamic_cast<Actor*>(this);
		if (actor != NULL)
			actor->UpdateSee();
		//else if (Region* region = dynamic_cast<Region*>(this)) {
		//	region->CheckObjectsInside();
		//}
		if (fWaitTime > 0)
			fWaitTime -= 15;
		else if (IsInsideVisibleArea() || dynamic_cast<AreaRoom*>(this)) {
			_ExecuteScripts(8);
		}		
	}
	
	if (fActions.size() != 0) {
		std::list<Action*>::iterator i = fActions.begin();
		while (i != fActions.end()) {
			Action& action = **i;
			if (action.Completed()) {
				delete *i;
				i = fActions.erase(i);
			} else {
				action();
				break;
			}
		}
	}
}


void
Object::AddAction(Action* action)
{
	fActions.push_back(action);
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
}


void
Object::AddScript(::Script* script)
{
	if (script != NULL)
		script->SetTarget(this);
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
Object::SetStale(bool stale)
{
	fStale = stale;
}


bool
Object::IsStale() const
{
	return fStale;
}


/* virtual */
void
Object::LastReferenceReleased()
{
	std::cout << Name() << "::LastReferenceReleased()" << std::endl;
	delete this;
}


void
Object::_ExecuteScripts(int32 maxLevel)
{
	try {
		for (int32 i = 0; i < maxLevel; i++) {		
			if (fScripts.at(i) != NULL) {
				if (!fScripts[i]->Execute())
					break;
			}
		}
	} catch (...) {
	}
}


