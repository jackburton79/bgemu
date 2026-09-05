/*
 * Object.cpp
 *
 *  Created on: 12/lug/2012
 *      Author: stefano
 */

#include "Object.h"

#include "Actor.h"
#include "Actions.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "BCSResource.h"
#include "Core.h"
#include "Game.h"
#include "Log.h"
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
	target_id(-1),
	round(0)
{
}


trigger_entry::trigger_entry(const std::string& trigName, Object* targetObject)
	:
	trigger_name(trigName),
	target_id(targetObject->GlobalID()),
	round(0)
{
}


// TODO: We cast to Actor very often in various methods.
// Either move the methods to actor, or merge the classes
Object::Object(const char* name, object_type objectType, const char* scriptName)
	:
	AutoDeletingReferenceable(),
	fName(name),
	fType(objectType),
	fGlobalID(-1),
	fTicks(0),
	fTicksIdle(0),
	fVisible(true),
	fActive(true),
	fIsInterruptable(true),
	fWaitTime(0),
	fCurrentActionParams(NULL),
	fLastTrigger(NULL),
	fArea(NULL),
	fRegion(NULL),
	fDisabled(false),
	fToDestroy(false)
{
	fScripts.fill(nullptr);

	if (scriptName != NULL) {
		::Script* script = Core::ExtractScript(scriptName);
		if (script != NULL)
			AddScript(script, SCRIPT_LEVEL_DEFAULT);
	}
	trigger_entry trig("OnCreation", this);
	trig.round = Core::Get()->ScriptRound();
	AddTrigger(trig);
}


Object::~Object()
{
	for (auto* script : fScripts)
		delete script;

	ClearActionList();
}


/* virtual */
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
		if (actor->CRE()->PermanentStatus() == STATE_DEAD)
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
	entry.round = Core::Get()->ScriptRound();
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

	if (fDisabled)
		scripts = false;

	if (scripts) {
		_HandleScripting(SCRIPT_LEVEL_COUNT);
	}

	if (fDisabled)
		return;

	// Dead actors don't act: their action list is cleared once at the
	// moment of death (Actor::ApplyDamage()), but this also guards
	// against anything still trying to queue a new action afterwards.
	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->IsState(STATE_DEAD))
		return;

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
Object::AddAction(action_params* params)
{
	SetActive(true);
	params->Acquire();

	if (IsInstantAction(params->id) && IsActionListEmpty()) {
		//std::cout << "action was instant and we execute it now!" << std::endl;
		fCurrentActionParams = params;
		fActionState = action_state();
		_ExecuteAction();
		return;
	}

	fActions.push_back(params);
}


void
Object::ExecuteActions()
{
	if (fWaitTime) {
		if (--fWaitTime)
			return;
	}

	// TODO: handle uninterruptable action

	 while (true) {
		if (fCurrentActionParams == NULL)
			PopNextAction();

		if (fCurrentActionParams == NULL)
			break;

		_ExecuteAction();

		// fCurrentActionParams is not completed, will
		// do another execution next time
		if (fCurrentActionParams != NULL)
			break;
	}
}


const action_params*
Object::CurrentAction() const
{
	return fCurrentActionParams;
}


const action_state*
Object::CurrentActionState() const
{
	return &fActionState;
}


void
Object::ClearCurrentAction()
{
	if (fCurrentActionParams != NULL) {
		fCurrentActionParams->Release();
		fCurrentActionParams = NULL;
	}
	SetInterruptable(true);
}


bool
Object::IsActionListEmpty() const
{
	return fCurrentActionParams == NULL && fActions.empty();
}


action_params*
Object::PopNextAction()
{
	if (!fActions.empty()) {
		fCurrentActionParams = fActions.front();
		fActions.pop_front();
		fActionState = action_state();
	}
	return fCurrentActionParams;
}


void
Object::ClearActionList()
{
	ClearCurrentAction();
	for (auto* params : fActions)
		params->Release();
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
	for (const auto &trigger : fTriggers) {
		if (trigger.trigger_name == trigName)
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
	for (const auto &entry : fTriggers) {
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
	for (const auto& entry :  fTriggers) {
		Object* object = Area()->GetObject(entry.target_id);
		std::cout << Name() << ": " << entry.trigger_name;
		if (object != NULL)
			std::cout << " -> " << object->Name();
		std::cout << std::endl;
	}
}


void
Object::RemoveExpiredTriggers()
{
	uint32 currentRound = Core::Get()->ScriptRound();
	fTriggers.remove_if([&] (const trigger_entry& t) {
							return currentRound > t.round;
						});
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
Object::AddScript(::Script* script, SCRIPT_LEVEL level)
{
	assert(level >= 0 && level < SCRIPT_LEVEL_COUNT);

	// Replace whatever was in that slot, if anything (e.g. CHANGEAISCRIPT
	// swapping the class-level script, or ActionOverride setting/clearing
	// the override-level one).
	delete fScripts[level];
	fScripts[level] = script;

	if (script != NULL)
		script->SetSender(this);
}


::Script*
Object::ScriptAt(SCRIPT_LEVEL level) const
{
	assert(level >= 0 && level < SCRIPT_LEVEL_COUNT);
	return fScripts[level];
}


void
Object::RemoveScript(SCRIPT_LEVEL level)
{
	assert(level >= 0 && level < SCRIPT_LEVEL_COUNT);
	delete fScripts[level];
	fScripts[level] = NULL;
}


void
Object::ClearScripts()
{
	for (auto*& script : fScripts) {
		delete script;
		script = NULL;
	}
}


void
Object::Disable()
{
	fDisabled = true;
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


void
Object::_HandleScripting(int32 maxLevel)
{
	if (sDebug)
		std::cout << Name() << ": _HandleScripting()" << std::endl;

	if (fTicks % 16 != GlobalID() % 16)
		return;

	if (sDebug)
	    std::cout << Name() << ": _HandleScripting() running, CutsceneMode="
	        << Core::Get()->CutsceneMode() << std::endl;

	bool runScripts = (fTicksIdle > 15) || IsActionListEmpty();

	Actor* actor = dynamic_cast<Actor*>(this);
	if (actor != NULL && actor->IsState(STATE_DEAD))
		return;

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

	RemoveExpiredTriggers();
	/*if (true)
		ClearTriggers();*/
}


void
Object::_ExecuteScripts(int32 maxLevel)
{
	if (!IsInterruptable())
		return;

	maxLevel = std::min<int32>(maxLevel, SCRIPT_LEVEL_COUNT);
	try {
		bool continuing = false;
		bool actionDone = false;
		for (int32 i = 0; i < maxLevel; i++) {
#if 0
			std::cout << "*** "<< Name() << ": script " << i << " ***" << std::endl;
#endif
			if (fScripts.at(i) == NULL)
				continue;

			fScripts[i]->Execute(continuing, actionDone);
			if (actionDone && !continuing) {
				//std::cout << Name() << ": script " << i << " returned false." << std::endl;
				break;
			}
		}
	} catch (std::exception& e) {
		std::cerr << Log::Red << e.what() << std::endl;
		std::cerr << Log::Normal;
	} catch (...) {
		std::cerr << Log::Red << "Exception while running script!" << std::endl;
		std::cerr << Log::Normal;
	}
}


void
Object::_ExecuteAction()
{
	SetInterruptable(false);

	const ActionDescriptor* descriptor = GetActionDescriptor(fCurrentActionParams->id);
	if (descriptor != NULL && descriptor->run != NULL) {
		descriptor->run(this, fCurrentActionParams, fActionState);
	} else {
		// No implementation for this action id (or the id isn't
		// even in the table): treat it as a no-op that completes
		// immediately.
		std::cerr << Log::Red << Name() << ": no implementation for action id "
			<< fCurrentActionParams->id << " (" << GetActionName(fCurrentActionParams->id)
			<< ")" << std::endl;
		std::cerr << Log::Normal;
		fActionState.completed = true;
	}

	// if completed, clear
	if (fCurrentActionParams != NULL && fActionState.completed) {
		ClearCurrentAction();
	}
}


void
Object::_ApplySpellEffects()
{
	for (auto i = fSpellEffects.begin(); i != fSpellEffects.end();) {
		SpellEffect* effect = *i;
		const EffectDescriptor* descriptor = GetEffectDescriptor(
				effect->Opcode());

		bool expired;
		if (descriptor != NULL && descriptor->run != NULL) {
			expired = descriptor->run(this, *effect);
		} else {
			// No native implementation for this opcode: treat it as a
			// no-op and drop it immediately, instead of leaving it stuck
			// on the object forever.
			std::cerr << Log::Red << Name()
					<< ": no implementation for spell effect opcode "
					<< effect->Opcode() << Log::Normal << std::endl;
			expired = true;
		}

		if (!expired)
			expired = effect->Tick();

		if (expired) {
			delete effect;
			i = fSpellEffects.erase(i);
		} else {
			i++;
		}
	}

}
