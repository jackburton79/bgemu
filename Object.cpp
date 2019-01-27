/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "AreaRoom.h"
#include "BCSResource.h"
#include "Core.h"
#include "Game.h"
#include "IDSResource.h"
#include "Object.h"
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
	fScript(NULL),
	fTicks(0),
	fVisible(true),
	fRegion(NULL),
	fStale(false)
{
	if (scriptName != NULL) {
		BCSResource* scriptResource = gResManager->GetBCS(scriptName);
		if (scriptResource != NULL) {
			::Script* script = scriptResource->GetScript();
			if (script != NULL)
				SetScript(script);
		}
		gResManager->ReleaseResource(scriptResource);
	}

	fTicks = rand() % 15;
}


Object::~Object()
{
	delete fScript;
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


void
Object::SetVariable(const char* name, int32 value)
{
	fVariables[name] = value;
}


int32
Object::GetVariable(const char* name)
{
	return fVariables[name];
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
		if (++fTicks >= 15) {
			fTicks = 0;
			// TODO: Make Object::Update() virtual and override
			// in subclasses to avoid dynamic casting
			Actor* actor = dynamic_cast<Actor*>(this);
			if (actor != NULL)
				actor->UpdateSee();
			else if (Region* region = dynamic_cast<Region*>(this)) {
				region->CheckObjectsInside();
			}
			if (fScript != NULL && IsInsideVisibleArea()) {
				if (!fScript->Execute())
					return;
			}
		}
	}
}


::Script*
Object::Script() const
{
	return fScript;
}


void
Object::SetScript(::Script* script)
{
	fScript = script;
	if (fScript != NULL)
		fScript->SetTarget(this);
}


/* virtual */
IE::point
Object::NearestPoint(const IE::point& point) const
{
	return Position();
}


void
Object::NewScriptRound()
{
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

