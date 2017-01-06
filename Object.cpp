/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Action.h"
#include "Actor.h"
#include "BackMap.h"
#include "BCSResource.h"
#include "Core.h"
#include "CreResource.h"
#include "Game.h"
#include "IDSResource.h"
#include "Object.h"
#include "Party.h"
#include "PathFind.h"
#include "Region.h"
#include "ResManager.h"
#include "Script.h"
#include "Room.h"
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
	fCurrentScriptRoundResults(NULL),
	fLastScriptRoundResults(NULL),
	fTileCell(NULL),
	fRegion(NULL),
	fStale(false)
{
	if (scriptName != NULL) {
		BCSResource* scriptResource = gResManager->GetBCS(scriptName);
		if (scriptResource != NULL) {
			Script* script = scriptResource->GetScript();
			if (script != NULL)
				SetScript(script);
		}
		gResManager->ReleaseResource(scriptResource);
	}

	fCurrentScriptRoundResults = new ScriptResults;

	fTicks = rand() % 15;
}


Object::~Object()
{
	std::list<Action*>::iterator i = fActions.begin();
	for (; i != fActions.end(); i++)
		delete *i;
	fActions.clear();

	delete fLastScriptRoundResults;
	delete fCurrentScriptRoundResults;
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
	return fName;
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
	fCurrentScriptRoundResults->fClicker = clicker;
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
	fRegion->CurrentScriptRoundResults()->SetEnteredActor(this);
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


void
Object::SetSeenBy(Actor* actor)
{
	fCurrentScriptRoundResults->SetSeenByObject(actor);
}


bool
Object::IsVisible() const
{
	return fVisible;
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
			if (fScript != NULL) {
				if (!fScript->Execute())
					return;
			}
		}
	}

	fTileCell = RoomContainer::Get()->BackMap()->TileAtPoint(Position());
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

	if (actor != NULL)
		actor->UpdateAnimation(actor->IsFlying());
}




void
Object::SetScript(Script* script)
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


::TileCell*
Object::TileCell() const
{
	return fTileCell;
}


// Checks if this object matches with the specified object_node.
// Also keeps wildcards in consideration. Used for triggers.
bool
Object::MatchNode(object_node* node) const
{
	if (IsName(node->name)
		&& IsClass(node->classs)
		&& IsRace(node->race)
		&& IsAlignment(node->alignment)
		&& IsGender(node->gender)
		&& IsGeneral(node->general)
		&& IsSpecific(node->specific)
		&& IsEnemyAlly(node->ea))
		return true;

	return false;
}


bool
Object::MatchWithOneInList(const std::vector<Object*>& vec) const
{
	std::vector<Object*>::const_iterator iter;
	for (iter = vec.begin(); iter != vec.end(); iter++) {
		if ((*iter)->IsEqual(this))
			return true;
	}

	return false;
}


/* static */
Object*
Object::GetMatchingObjectFromList(
		const std::vector<Reference<Object> >& vector, object_node* matches)
{
	std::vector<Reference<Object> >::const_iterator iter;
	for (iter = vector.begin(); iter != vector.end(); iter++) {
		if ((*iter).Target()->MatchNode(matches))
			return iter->Target();
	}
	return NULL;
}


/* static */
bool
Object::CheckIfNodesInList(const std::vector<object_node*>& nodeList,
		const std::vector<Object*>& vector)
{
	//node->Print();

	std::vector<Object*>::const_iterator iter;
	std::vector<object_node*>::const_iterator nodeIter;
	for (iter = vector.begin(); iter != vector.end(); iter++) {
		for (nodeIter = nodeList.begin(); nodeIter != nodeList.end(); nodeIter++) {
			if ((*iter)->MatchNode(*nodeIter))
				return true;
		}
	}
	return false;
}


bool
Object::IsEqual(const Object* objectB) const
{
	const Actor* actorA = dynamic_cast<const Actor*>(this);
	const Actor* actorB = dynamic_cast<const Actor*>(objectB);
	if (actorA == NULL || actorB == NULL)
		return false;

	CREResource* creA = actorA->CRE();
	CREResource* creB = actorB->CRE();

	if (::strcasecmp(actorA->Name(), actorB->Name()) == 0
		&& (creA->Class() == creB->Class())
		&& (creA->Race() == creB->Race())
		&& (creA->Alignment() == creB->Alignment())
		&& (creA->Gender() == creB->Gender())
		&& (creA->General() == creB->General())
		&& (creA->Specific() == creB->Specific())
		&& (creA->EnemyAlly(), creB->EnemyAlly()))
		return true;
	return false;
}


/* static */
bool
Object::IsDummy(const object_node* node)
{
	if (node->ea == 0
			&& node->general == 0
			&& node->race == 0
			&& node->classs == 0
			&& node->specific == 0
			&& node->gender == 0
			&& node->alignment == 0
			//&& node->faction == 0
			//&& node->team == 0
			&& node->identifiers[0] == 0
			&& node->identifiers[1] == 0
			&& node->identifiers[2] == 0
			&& node->identifiers[3] == 0
			&& node->identifiers[4] == 0
			&& node->name[0] == '\0'
			) {
		return true;
	}

	return false;
}


Object*
Object::ResolveIdentifier(const int id) const
{
	std::string identifier = IDTable::ObjectAt(id);
	if (identifier == "MYSELF")
		return const_cast<Object*>(this);
	// TODO: Implement more identifiers
	if (identifier == "NEARESTENEMYOF")
		return Core::Get()->GetNearestEnemyOf(this);
	// TODO: Move that one here ?
	// Move ResolveIdentifier elsewhere ?
	if (identifier == "LASTTRIGGER")
		return fScript->LastTrigger();
	if (identifier == "LASTATTACKEROF")
		return LastScriptRoundResults()->LastAttacker();

	std::cout << "ResolveIdentifier: UNIMPLEMENTED(" << id << ") = ";
	std::cout << identifier << std::endl;
	return NULL;
}


bool
Object::IsEnemyOf(const Object* object) const
{
	// TODO: Implement correctly
	const Actor* thisActor = dynamic_cast<const Actor*>(this);
	if (thisActor == NULL) {
		std::cout << "Not an actor" << std::endl;
		return false;
	}

	uint8 enemy = IDTable::EnemyAllyValue("ENEM");
	uint8 pc = IDTable::EnemyAllyValue("PC");
	// TODO: Is this correct ? I have no idea.
	return (object->IsEnemyAlly(enemy) 	&& thisActor->IsEnemyAlly(pc))
			|| (object->IsEnemyAlly(pc) && thisActor->IsEnemyAlly(enemy));
}


bool
Object::IsName(const char* name) const
{
	if (name[0] == '\0' || !strcasecmp(name, Name()))
		return true;
	return false;
}


bool
Object::IsClass(int c) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (c == 0 || c == cre->Class())
		return true;

	return false;
}


bool
Object::IsRace(int race) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (race == 0 || race == cre->Race())
		return true;

	return false;
}


bool
Object::IsGender(int gender) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (gender == 0 || gender == cre->Gender())
		return true;

	return false;
}


bool
Object::IsGeneral(int general) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (general == 0 || general == cre->General())
		return true;

	return false;
}


bool
Object::IsSpecific(int specific) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (specific == 0 || specific == cre->Specific())
		return true;

	return false;
}


bool
Object::IsAlignment(int alignment) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	CREResource* cre = actor->CRE();
	if (alignment == 0 || alignment == cre->Alignment())
		return true;

	return false;
}


bool
Object::IsEnemyAlly(int ea) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	if (ea == 0)
		return true;

	CREResource* cre = actor->CRE();
	if (ea == cre->EnemyAlly())
		return true;

	std::string eaString = IDTable::EnemyAllyAt(ea);

	if (eaString == "PC") {
		if (Game::Get()->Party()->HasActor(actor))
			return true;
	} else if (eaString == "GOODCUTOFF") {
		if (cre->EnemyAlly() <= ea)
			return true;
	} else if (eaString == "EVILCUTOFF") {
		if (cre->EnemyAlly() >= ea)
			return true;
	}

	return false;
}


void
Object::SetEnemyAlly(int ea)
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return;

	actor->CRE()->SetEnemyAlly(ea);
}


bool
Object::IsState(int state) const
{
	const Actor* actor = dynamic_cast<const Actor*>(this);
	if (actor == NULL)
		return false;

	if (actor->CRE()->PermanentStatus() & state)
		return true;

	return false;
}


void
Object::NewScriptRound()
{
	std::cout << Name() << " NewScriptRound" << std::endl;
	delete fLastScriptRoundResults;
	fLastScriptRoundResults = fCurrentScriptRoundResults;
	fCurrentScriptRoundResults = new ScriptResults;
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


ScriptResults*
Object::CurrentScriptRoundResults() const
{
	return fCurrentScriptRoundResults;
}


ScriptResults*
Object::LastScriptRoundResults() const
{
	return fLastScriptRoundResults;
}


void
Object::AttackTarget(Object* target)
{
	target->fCurrentScriptRoundResults->fAttackers.push_back(this);
}


/* virtual */
void
Object::LastReferenceReleased()
{
	std::cout << Name() << "::LastReferenceReleased()" << std::endl;
	delete this;
}


// ScriptResults
ScriptResults::ScriptResults()
	:
	fClicker(NULL),
	fShouted(-1)
{
}


const std::vector<Reference<Object> >
ScriptResults::Attackers() const
{
	return fAttackers;
}


const std::vector<Reference<Object> >
ScriptResults::Hitters() const
{
	return fHitters;
}


const std::vector<Reference<Object> >
ScriptResults::EnteredActors() const
{
	return fEnteredActors;
}


/*
const
std::vector<Object*>&
ScriptResults::SeenList() const
{
	return fSeen;
}*/


Object*
ScriptResults::LastAttacker() const
{
	const int32 numAttackers = fAttackers.size();
	if (numAttackers == 0)
		return NULL;

	return Attackers()[numAttackers - 1].Target();
}


void
ScriptResults::SetAttackedBy(Object* object)
{
	fAttackers.push_back(object);
}

	
void
ScriptResults::SetObjectSaw(Object* object)
{
	fSeenList.push_back(object);
}


void
ScriptResults::SetSeenByObject(Object* object)
{
	fSeenByList.push_back(object);
}


void
ScriptResults::SetEnteredActor(Object* object)
{
	fEnteredActors.push_back(object);	
}


int32
ScriptResults::CountAttackers() const
{
	return fAttackers.size();
}


Object*
ScriptResults::AttackerAt(int32 i) const
{
	return Attackers()[i].Target();
}


Object*
ScriptResults::Clicker() const
{
	return fClicker.Target();
}


int
ScriptResults::Shouted() const
{
	return fShouted;
}
