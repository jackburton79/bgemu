#include "Action.h"
#include "Actor.h"
#include "Animation.h"
#include "Core.h"
#include "Door.h"
#include "Game.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Parsing.h"
#include "RectUtils.h"
#include "Region.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Script.h"
#include "SPLResource.h"
#include "Timer.h"
// TODO: Remove this dependency
#include "TLKResource.h"

#include <algorithm>
#include <cassert>
#include <cxxabi.h>
#include <typeinfo>

static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= 5 * 2)
		&& (std::abs(pointA.y - pointB.y) <= 5 * 2);
}



Action::Action(Object* object, action_params* node)
    :
	fObject(object),
	fActionParams(node),
	fInitiated(false),
	fCompleted(false)
{
	assert(fObject != NULL);
	fObject->Acquire();
}


Action::~Action()
{
	fObject->Release();
}


bool
Action::Initiated() const
{
	return fInitiated;
}


void
Action::SetInitiated()
{
	fInitiated = true;
}


bool
Action::Completed() const
{
    return fCompleted;
}


void
Action::SetCompleted()
{
	fCompleted = true;
}


bool
Action::IsInstant() const
{
	IDSResource* instants = gResManager->GetIDS("INSTANT");
	if (instants == NULL)
		return false;
	bool returnValue = instants->StringForID(fActionParams->id) != "";
	gResManager->ReleaseResource(instants);
	return returnValue;
}


std::string
Action::Name() const
{
	int status;
	char* demangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
	std::string name = demangled;
	free(demangled);
	return name;
}


// SetGlobalAction
ActionSetGlobal::ActionSetGlobal(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionSetGlobal::operator()()
{
	std::string variableScope;
	std::string variableName;
	Variables::GetNameAndScope(fActionParams->string1, variableScope, variableName);
	if (variableScope.compare("LOCALS") == 0) {
		if (fObject != NULL)
			fObject->SetVariable(variableName.c_str(),
					fActionParams->integer1);
	} else {
		// TODO: Check for AREA variables
		Core::Get()->Vars().Set(fActionParams->string1, fActionParams->integer1);
	}
	SetCompleted();
}


// CreateCreatureAction
ActionCreateCreature::ActionCreateCreature(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionCreateCreature::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	IE::point point = fActionParams->where;
	if (point.x == -1 && point.y == -1) {
		Actor* thisActor = dynamic_cast<Actor*>(fObject);
		if (thisActor != NULL) {
			point = thisActor->Position();
			point.x += Core::RandomNumber(-20, 20);
			point.y += Core::RandomNumber(-20, 20);
		}
	}
	std::cout << "create actor at " << point.x << ", " << point.y << std::endl;

	Actor* actor = new Actor(fActionParams->string1, point, fActionParams->integer1);
	Core::Get()->AddActorToCurrentArea(actor);
	//core->SetActiveActor(actor);
	SetCompleted();
}


// CreateCreatureImpassableAction
ActionCreateCreatureImpassable::ActionCreateCreatureImpassable(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionCreateCreatureImpassable::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	Actor* actor = new Actor(fActionParams->string1,
						fActionParams->where, fActionParams->integer1);
	std::cout << "Created actor (IMPASSABLE) " << fActionParams->string1 << " on ";
	std::cout << fActionParams->where.x << ", " << fActionParams->where.y << std::endl;
	//actor->SetDestination(fActionParams->where);
	Core::Get()->AddActorToCurrentArea(actor);
	SetCompleted();
	//core->SetActiveActor(actor);
}


// TriggerActivationAction
ActionTriggerActivation::ActionTriggerActivation(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionTriggerActivation::operator()()
{
	Region* region = dynamic_cast<Region*>(Script::GetTargetObject(fObject, fActionParams));
	if (region != NULL)
		region->ActivateTrigger();
	SetCompleted();
}


// ActionUnlock
ActionUnlock::ActionUnlock(Object* object, action_params* node)
	:
	Action(object, node)
{
}


void
ActionUnlock::operator()()
{
	Object* sender = Script::GetSenderObject(fObject, fActionParams);
	Object* target = Script::GetTargetObject(sender, fActionParams);
	Door* door = dynamic_cast<Door*>(target);
	if (door == NULL) {
		std::cerr << "NULL DOOR!!! MEANS THE OBJECT IS NOT A DOOR" << std::endl;
		SetCompleted();
		return;
	}

	door->Unlock();

	SetCompleted();
}


// DestroySelfAction
ActionDestroySelf::ActionDestroySelf(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionDestroySelf::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	Object* object = Script::GetSenderObject(fObject, fActionParams);
	object->DestroySelf();
	SetCompleted();
}


// ForceSpell
ActionForceSpell::ActionForceSpell(Object* object, action_params* node)
	:
	Action(object, node),
	fDuration(50)
{
	fStart = Timer::Ticks();
}


/* virtual */
void
ActionForceSpell::operator()()
{
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (sender == NULL) {
		std::cerr << "ActionForceSpell:: NO sender Actor" << std::endl;
		SetCompleted();
		return;
	}
	//sender->Print();

	//Object* target = Script::GetTargetObject(sender, fActionParams);
	//target->Print();

	if (!Initiated()) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(fActionParams->integer1).c_str();
		std::string spellResourceName = SPLResource::GetSpellResourceName(fActionParams->integer1);
		gResManager->ReleaseResource(spellIDS);

		std::cout << "spell: " << spellName << std::endl;
		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		std::cout << "Casting time: " << spellResource->CastingTime() << std::endl;

		// TODO: Not sure if it's correct. CastingTime is 1/10 of round.
		// Round takes 6 seconds (in BG)
		// AI update every 15 ticks (do we respect this ? don't know)
		fDuration = spellResource->CastingTime() * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "fDuration:" << fDuration << std::endl;
		std::cout << spellResource->DescriptionIdentifiedRef() << std::endl;
		std::cout << spellResource->DescriptionUnidentifiedRef() << std::endl;
		gResManager->ReleaseResource(spellResource);
		sender->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		SetInitiated();
	}

	// TODO: only for testing
	if (fDuration-- == 0) {
		// TODO: There should be a way to set the previous animation action,
		// because we don't know here if ACT_STANDING is the correct one

		// TODO: Play final cast animation
		sender->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		SetCompleted();
		std::cout << "duration:" << (Timer::Ticks() - fStart) << std::endl;
	}
}


// MoveBetweenAreasEffect
ActionMoveBetweenAreasEffect::ActionMoveBetweenAreasEffect(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionMoveBetweenAreasEffect::operator()()
{
	if (!Initiated()) {
		SetInitiated();
		Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
		if (actor != NULL) {
			std::cout << "area:" << fActionParams->string1 << std::endl;
			actor->SetPosition(fActionParams->where);
			actor->SetOrientation(fActionParams->integer1);
		}
		SetCompleted();
	}
}


// PlayDeadAction
ActionPlayDead::ActionPlayDead(Object* object, action_params* node)
	:
	Action(object, node)
{
	fDuration = fActionParams->integer1 * AI_UPDATE_FREQ;
}


/* virtual */
void
ActionPlayDead::operator()()
{
	if (!Initiated()) {
		SetInitiated();
		Actor* actor = dynamic_cast<Actor*>(fObject);
		if (actor == NULL) {
			SetCompleted();
			return;
		}
		actor->SetAnimationAction(ACT_DIE);
	}
	
	if (fDuration-- <= 0)
		SetCompleted();
}


// SetInterruptableAction
ActionSetInterruptable::ActionSetInterruptable(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionSetInterruptable::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	fObject->SetInterruptable(fActionParams->integer1 == 1);
	SetCompleted();
}


// WalkTo
ActionWalkTo::ActionWalkTo(Object* object, action_params* node, bool canInterrupt)
	:
	Action(object, node),
	fInterruptable(canInterrupt)
{
}


/* virtual */
void
ActionWalkTo::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (!Initiated()) {	
		actor->SetDestination(fActionParams->where);
		if (!fInterruptable)
			actor->SetInterruptable(false);
		SetInitiated();
	}

	if (actor->Position() == actor->Destination()) {
		SetCompleted();
		return;
	}
}


// WalkToObject
ActionWalkToObject::ActionWalkToObject(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionWalkToObject::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL) {
		SetCompleted();
		return;
	}

	Object* target = Script::GetTargetObject(actor, fActionParams);
	if (target == NULL) {
		SetCompleted();
		return;
	}
	
	//target->Acquire();
	
	IE::point destination = target->NearestPoint(actor->Position());
	if (!PointSufficientlyClose(actor->Position(), destination))
		actor->SetDestination(destination);

	if (actor->Position() == actor->Destination()) {
		SetCompleted();
		//target->Release();
		return;
	}
}

// RandomFly
ActionRandomFly::ActionRandomFly(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionRandomFly::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL)
		return;

	// TODO: Fly
	int16 randomX = Core::RandomNumber(-50, 50);
	int16 randomY = Core::RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	if (!PointSufficientlyClose(actor->Position(), destination))
		actor->SetDestination(destination, true);

	if (actor->Position() == actor->Destination())
		SetCompleted();
}


// FlyTo
ActionFlyTo::ActionFlyTo(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionFlyTo::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL)
		return;

	if (!Initiated()) {
		actor->SetDestination(fActionParams->where, true);
		SetInitiated();
	}

	if (actor->Position() == actor->Destination()) {
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	actor->SetAnimationAction(ACT_WALKING);
	actor->MoveToNextPointInPath(true);
}


// IncrementGlobal
ActionIncrementGlobal::ActionIncrementGlobal(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionIncrementGlobal::operator()()
{
	Core* core = Core::Get();
	std::string variableScope;
	std::string variableName;
	Variables::GetNameAndScope(fActionParams->string1, variableScope, variableName);
	int32 value = core->Vars().Get(fActionParams->string1);
	core->Vars().Set(fActionParams->string1, value + fActionParams->integer1);
	SetCompleted();
}


// RandomWalk
ActionRandomWalk::ActionRandomWalk(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionRandomWalk::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL)
		return;

	int16 randomX = Core::RandomNumber(-50, 50);
	int16 randomY = Core::RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	if (!PointSufficientlyClose(actor->Position(), destination))
		actor->SetDestination(destination);

	if (actor->Position() == actor->Destination())
		SetCompleted();
}


// Wait
ActionWait::ActionWait(Object* object, action_params* node)
	:
	Action(object, node),
	fWaitTime(0)
{
}


/* virtual */
void
ActionWait::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	if (!Initiated()) {
		SetInitiated();
		fWaitTime = fActionParams->integer1 * AI_UPDATE_FREQ;
		return;
	}
	
	if (--fWaitTime <= 0)
		SetCompleted();
}


// SmallWait
ActionSmallWait::ActionSmallWait(Object* object, action_params* node)
	:
	Action(object, node),
	fWaitTime(0)
{
}


/* virtual */
void
ActionSmallWait::operator()()
{
	// TODO: Sometimes there is a different sender object.
	// Find a way to execute this action on the correct sender
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	//Object* object = Script::FindObject(fObject, fActionParams);
	//if (object != NULL)
	//	object->SetWaitTime(fActionParams->integer1);
	if (!Initiated()) {
		SetInitiated();
		fWaitTime = fActionParams->integer1;
		return;
	}
	
	if (--fWaitTime <= 0)
		SetCompleted();
}


// OpenDoor
ActionOpenDoor::ActionOpenDoor(Object* sender, action_params* node)
	:
	Action(sender, node)
{
}


/* virtual */
void
ActionOpenDoor::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL) {
		std::cerr << "NULL ACTOR!!!" << std::endl;
		return;
	}

	Object* target = Script::GetTargetObject(actor, fActionParams);
	Door* door = dynamic_cast<Door*>(target);
	if (door == NULL) {
		std::cerr << "NULL DOOR!!! MEANS THE OBJECT IS NOT A DOOR" << std::endl;
		SetCompleted();
		return;
	}

	std::cout << "actor " << actor->Name() << " opens " << door->Name() << std::endl;
	if (!door->Opened()) {
		door->Open(actor);
		SetCompleted();
	}
}


// DisplayMessage
ActionDisplayMessage::ActionDisplayMessage(Object* sender, action_params* node)
	:
	Action(sender, node)
{
}


/* virtual */
void
ActionDisplayMessage::operator()()
{
	std::cout << "DisplayMessage:: ";
	std::string dialogText = IDTable::GetDialog(fActionParams->integer1);
	std::cout << dialogText << std::endl;
	Core::Get()->DisplayMessage(NULL, dialogText.c_str());
	SetCompleted();
}


// PlayMovie
ActionPlayMovie::ActionPlayMovie(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionPlayMovie::operator()()
{
	Core::Get()->PlayMovie(fActionParams->string1);
	SetCompleted();
}


// Attack
ActionAttack::ActionAttack(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionAttack::operator()()
{
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (sender == NULL)
		return;
	
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, fActionParams));
	if (target == NULL){
		SetCompleted();
		return;
	}

	IE::point point = target->NearestPoint(sender->Position());
	if (!PointSufficientlyClose(sender->Position(), point))
		sender->SetDestination(point);

	if (sender->Position() != sender->Destination()) {
		sender->SetAnimationAction(ACT_WALKING);
		sender->MoveToNextPointInPath(sender->IsFlying());
	} else {
		sender->SetAnimationAction(ACT_ATTACKING);
		sender->AttackTarget(target);
		SetCompleted();
	}
}


// RunAwayFrom
ActionRunAwayFrom::ActionRunAwayFrom(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionRunAwayFrom::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor == NULL)
		return;

	// TODO: Improve.
	// TODO: We are recalculating this every time. Is it correct ?
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(actor, fActionParams));
	if (target == NULL){
		SetCompleted();
		return;
	}
	
	// TODO: Improve implementation
	if (Core::Get()->Distance(actor, target) < 200) {
		IE::point point = PointAway(actor, target);
		if (actor->Destination() != point) {
			actor->SetDestination(point);
		}
	}

	if (actor->Position() == actor->Destination()) {
		SetCompleted();
		actor->SetAnimationAction(ACT_STANDING);
	} else {
		actor->SetAnimationAction(ACT_WALKING);
		actor->MoveToNextPointInPath(actor->IsFlying());
	}
}


IE::point
ActionRunAwayFrom::PointAway(Actor* actor, Actor* target)
{
	IE::point targetPos = target->NearestPoint(actor->Position());
	IE::point actorPos = actor->Position();
	if (targetPos.x > actorPos.x)
		actorPos.x -= 150;
	else if (targetPos.x < actorPos.x)
		actorPos.x += 150;

	if (targetPos.y > actorPos.y)
		actorPos.y -= 150;
	else if (targetPos.y < actorPos.y)
		actorPos.y += 150;

	return actorPos;
}


// DialogAction
ActionDialog::ActionDialog(Object* source, action_params* node)
	:
	Action(source, node)
{
	std::cout << "Dialogue::Dialogue()" << std::endl;
}


/* virtual */
void
ActionDialog::operator()()
{
	Object* object = Script::GetSenderObject(fObject, fActionParams);
	if (object == NULL) {
		SetCompleted();
		return;
	}
	
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(object, fActionParams));
	if (target == NULL) {
		SetCompleted();
		return;
	}

	if (!Initiated()) {
		Actor* actor = dynamic_cast<Actor*>(object);
		if (actor != NULL)
			Game::Get()->InitiateDialog(actor, target);
		SetInitiated();
	}
	std::cout << "Actor " << object->Name();
	std::cout << " will talk to " << target->Name() << std::endl;
	// TODO: Some dialogue action require the actor to be near the target,
	// others do not. Must be able to differentiate
/*
	const IE::point point = fTarget->Target()->NearestPoint(fActor.Target()->Position());
	if (!PointSufficientlyClose(fActor.Target()->Destination(), point))
		fActor.Target()->SetDestination(point);
*/

/*
	if (!PointSufficientlyClose(fActor.Target()->Position(), fTarget->Target()->Position())) {
		fActor.Target()->SetAnimationAction(ACT_WALKING);
		fActor.Target()->MoveToNextPointInPath(fActor.Target()->IsFlying());
	} else {
*/
		//SetAnimationAction(ACT_STANDING);
	SetCompleted();
	//}
}


// ActionSetEnemyAlly
ActionSetEnemyAlly::ActionSetEnemyAlly(Object* actor, action_params* node)
	:
	Action(actor, node)
{
}


/* virtual */
void
ActionSetEnemyAlly::operator()()
{
	uint32 id = IDTable::EnemyAllyValue("ENEM");
	// TODO: Correct ? or should we get the sender object ?
	Actor* actor = dynamic_cast<Actor*>(fObject);
	if (actor != NULL)
		actor->SetEnemyAlly(id);
	SetCompleted();
}


// FadeToColorAction
ActionFadeToColor::ActionFadeToColor(Object* object, action_params* node)
	:
	Action(object, node),
	fCurrentValue(0),
	fTargetValue(0),
	fStepValue(1)
{
}


/* virtual */
void
ActionFadeToColor::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	if (!Initiated()) {
		SetInitiated();
		fCurrentValue = 255;
		fTargetValue = 0;
		fStepValue = (fCurrentValue - fTargetValue) / fActionParams->where.x;
	}
	
	GraphicsEngine::Get()->SetFade(fCurrentValue);
	if (fCurrentValue > fTargetValue)
		fCurrentValue -= fStepValue;
	else
		SetCompleted();
}


// FadeFromColorAction
ActionFadeFromColor::ActionFadeFromColor(Object* object, action_params* node)
	:
	Action(object, node),
	fCurrentValue(0),
	fTargetValue(0),
	fStepValue(1)
{
}


/* virtual */
void
ActionFadeFromColor::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	if (!Initiated()) {
		SetInitiated();
		fCurrentValue = 0;
		fTargetValue = 255;
		fStepValue = fTargetValue / fActionParams->where.x;
	}
	
	GraphicsEngine::Get()->SetFade(fCurrentValue);
	if (fCurrentValue < fTargetValue)
		fCurrentValue += fStepValue;
	else
		SetCompleted();
}


// MoveViewPoint
ActionMoveViewPoint::ActionMoveViewPoint(Object* object, action_params* node)
	:
	Action(object, node),
	fScrollSpeed(10000)
{	
}


/* virtual */
void
ActionMoveViewPoint::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	//SetCompleted();
	//return;
	if (!Initiated()) {
		SetInitiated();
		fDestination = fActionParams->where;
		Core::Get()->CurrentRoom()->SanitizeOffsetCenter(fDestination);
		switch (fActionParams->integer1) {
			case 1:
				fScrollSpeed = 10;
				break;
			case 2:
				fScrollSpeed = 20;
				break;
			case 3:
				fScrollSpeed = 40;
				break;
			case 4:
				fScrollSpeed = 80;
				break;
			case 0:		
			default:		
				fScrollSpeed = 10000;
				break;
		}
	}
	
	RoomBase* room = Core::Get()->CurrentRoom();
	IE::point offset = room->AreaCenterPoint();
	const int16 step = fScrollSpeed;
	if (offset != fDestination) {
		if (offset.x > fDestination.x)
			offset.x = std::max((int16)(offset.x - step), fDestination.x);
		else if (offset.x < fDestination.x)
			offset.x = std::min((int16)(offset.x + step), fDestination.x);
		
		if (offset.y > fDestination.y)
			offset.y = std::max((int16)(offset.y - step), fDestination.y);
		else if (offset.y < fDestination.y)
			offset.y = std::min((int16)(offset.y + step), fDestination.y);
		room->SetAreaOffsetCenter(offset);
		std::cout << std::dec;
		std::cout << "offset: " << offset.x << ", " << offset.y << std::endl;
		std::cout << "fDestination: " << fDestination.x << ", " << fDestination.y << std::endl;
	} else
		SetCompleted();
}


ActionScreenShake::ActionScreenShake(Object* object, action_params* node)
	:
	Action(object, node)
{
	fDuration = fActionParams->integer1;
}


/* virtual */
void
ActionScreenShake::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	if (!Initiated()) {
		SetInitiated();
		if (fObject != NULL)
			fObject->SetWaitTime(fDuration);
		fOffset = fActionParams->where;
	}
	
	GFX::point point = { 0, 0 };
	if (fDuration-- == 0) {	
		GraphicsEngine::Get()->SetRenderingOffset(point);	
		SetCompleted();
		return;
	}
	
	point.x = fOffset.x;	
	point.y = fOffset.y;
	
	GraphicsEngine::Get()->SetRenderingOffset(point);
	fOffset.x = -fOffset.x;
	fOffset.y = -fOffset.y;
}


// StartCutsceneModeAction
ActionStartCutsceneMode::ActionStartCutsceneMode(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionStartCutsceneMode::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	Core::Get()->StartCutsceneMode();
	SetCompleted();
}


// EndCutsceneModeAction
ActionEndCutsceneMode::ActionEndCutsceneMode(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionEndCutsceneMode::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	Core::Get()->EndCutsceneMode();
	SetCompleted();
}


// ActionClearAllActions
ActionClearAllActions::ActionClearAllActions(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionClearAllActions::operator()()
{
	Core::Get()->ClearAllActions();
	SetCompleted();
}


// ActionSetGlobalTimer
ActionSetGlobalTimer::ActionSetGlobalTimer(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionSetGlobalTimer::operator()()
{
	std::string timerName;
	// TODO: We append the timer name to the area name,
	// check if it's okay
	timerName.append(fActionParams->string2).append(fActionParams->string1);
	GameTimer::Add(timerName.c_str(), fActionParams->integer1);
	SetCompleted();
}


// ActionStartCutscene
ActionStartCutscene::ActionStartCutscene(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionStartCutscene::operator()()
{
	if (fObject == NULL)
		std::cerr << "ActionStartCutscene: NULL OBJECT" << std::endl;
	Core::Get()->StartCutscene(fActionParams->string1);
	SetCompleted();
}


// HideGUIAction
ActionHideGUI::ActionHideGUI(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionHideGUI::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	GUI::Get()->Hide();
	SetCompleted();
}


// UnhideGUIAction
ActionUnhideGUI::ActionUnhideGUI(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionUnhideGUI::operator()()
{
	if (fObject == NULL)
		std::cerr << "NULL OBJECT" << std::endl;
	GUI::Get()->Show();
	SetCompleted();
}


ActionDisplayString::ActionDisplayString(Object* object, action_params* node)
	:
	Action(object, node)
{
	fDuration = fActionParams->integer1;
}


/* virtual */
void
ActionDisplayString::operator()()
{
	if (!Initiated()) {
		SetInitiated();
		std::string string = IDTable::GetDialog(fActionParams->integer1); 
		GUI::Get()->DisplayString(string, fActionParams->where.x, fActionParams->where.y, fDuration * AI_UPDATE_FREQ);
	}
	
	if (fDuration-- <= 0)
		SetCompleted();
}


ActionDisplayStringHead::ActionDisplayStringHead(Object* object, action_params* node)
	:
	Action(object, node)
{
	fDuration = 100; //??
}


/* virtual */
void
ActionDisplayStringHead::operator()()
{
	if (!Initiated()) {
		SetInitiated();
		Object* sender = Script::GetSenderObject(fObject, fActionParams);
		Actor* actor = dynamic_cast<Actor*>(Script::GetTargetObject(sender, fActionParams));
		if (actor == NULL) {
			std::cerr << "ActionDisplayHead: no TARGET!!!" << std::endl;
			SetCompleted();
			return;
		}
		TLKEntry* tlkEntry = IDTable::GetTLK(fActionParams->integer1);
		actor->SetText(tlkEntry->text);
		delete tlkEntry;
	}
	if (fDuration-- <= 0) {
		Object* sender = Script::GetSenderObject(fObject, fActionParams);
		Actor* actor = dynamic_cast<Actor*>(Script::GetTargetObject(sender, fActionParams));
		if (actor != NULL)
			actor->SetText("");
		SetCompleted();
	}
}


// ChangeOrientationExtAction
ActionChangeOrientationExt::ActionChangeOrientationExt(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionChangeOrientationExt::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	if (actor != NULL)
		actor->SetOrientation(fActionParams->integer1);
	SetCompleted();
}


// FaceObject
ActionFaceObject::ActionFaceObject(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionFaceObject::operator()()
{
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	Object* target = Script::GetTargetObject(sender, fActionParams);
	if (sender == NULL || target == NULL) {
		std::cerr << "FaceObject(): NULL object" << std::endl;
		SetCompleted();
		return;
	}

	const IE::rect objectFrame = target->Frame();
	IE::point point;
	point.x = objectFrame.Width() / 2;
	point.y = objectFrame.Height() / 2;
	sender->SetOrientation(point);
	SetCompleted();
}


// CreateVisualEffect
ActionCreateVisualEffect::ActionCreateVisualEffect(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionCreateVisualEffect::operator()()
{
	Core::Get()->PlayEffect(fActionParams->string1, fActionParams->where);
	SetCompleted();
}


// CreateVisualEffectObject
ActionCreateVisualEffectObject::ActionCreateVisualEffectObject(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionCreateVisualEffectObject::operator()()
{
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fObject, fActionParams));
	Object* target = Script::GetTargetObject(sender, fActionParams);
	IE::point point;

	if (target->Type() == Object::ACTOR) {
		point = dynamic_cast<Actor*>(target)->Position();
	} else {
		point.x = target->Frame().x_max - target->Frame().x_min;
		point.y = target->Frame().y_max - target->Frame().y_min;
	}

	Core::Get()->PlayEffect(fActionParams->string1, point);
	SetCompleted();
}
