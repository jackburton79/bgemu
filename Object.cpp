/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Action.h"
#include "Actor.h"
#include "AreaRoom.h"
#include "BackMap.h"
#include "BCSResource.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "IDSResource.h"
#include "Object.h"
#include "Party.h"
#include "PathFind.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"
#include "RoundResults.h"
#include "TileCell.h"

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
	fTileCell(NULL),
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
	const Actor* thisActor = dynamic_cast<const Actor*>(this);
	if (thisActor == NULL)
	    return;
	CREResource* cre = thisActor->CRE();
	std::cout << "*** " << thisActor->Name() << " ***" << std::endl;
	std::cout << "Gender: " << IDTable::GenderAt(cre->Gender());
	std::cout << " (" << (int)cre->Gender() << ")" << std::endl;
	std::cout << "Class: " << IDTable::ClassAt(cre->Class());
	std::cout << " (" << (int)cre->Class() << ")" << std::endl;
	std::cout << "Race: " << IDTable::RaceAt(cre->Race());
	std::cout << " (" << (int)cre->Race() << ")" << std::endl;
	std::cout << "EA: " << IDTable::EnemyAllyAt(cre->EnemyAlly());
	std::cout << " (" << (int)cre->EnemyAlly() << ")" << std::endl;
	std::cout << "General: " << IDTable::GeneralAt(cre->General());
	std::cout << " (" << (int)cre->General() << ")" << std::endl;
	std::cout << "Specific: " << IDTable::SpecificAt(cre->Specific());
	std::cout << " (" << (int)cre->Specific() << ")" << std::endl;
	std::cout << "*********" << std::endl;
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
	Actor* actor = dynamic_cast<Actor*>(this);
	if (scripts) {
		if (++fTicks >= 15) {
			fTicks = 0;
			// TODO: Make Object::Update() virtual and override
			// in subclasses to avoid dynamic casting
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
//	std::cout << Name() << " NewScriptRound" << std::endl;
//	delete fLastScriptRoundResults;
//	fLastScriptRoundResults = fCurrentScriptRoundResults;
//	fCurrentScriptRoundResults = new ScriptResults;
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

