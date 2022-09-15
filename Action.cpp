#include "Action.h"

#include "Actor.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "Core.h"
#include "Door.h"
#include "Effect.h"
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
#include "SpellEffect.h"
#include "SPLResource.h"
#include "Timer.h"
// TODO: Remove this dependency
#include "TLKResource.h"

#include <algorithm>
#include <cassert>
#include <cxxabi.h>
#include <sstream>
#include <typeinfo>

static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= 5 * 2)
		&& (std::abs(pointA.y - pointB.y) <= 5 * 2);
}


Action::Action(Object* object, action_params* node)
    :
	fSender(object),
	fActionParams(node),
	fInitiated(false),
	fCompleted(false)
{
	assert(fSender != NULL);
	assert (fActionParams != NULL);
	fSender->Acquire();
	fActionParams->Acquire();
}


Action::~Action()
{
	fSender->Release();
	fActionParams->Release();
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
		if (fSender != NULL)
			fSender->SetVariable(variableName.c_str(),
					fActionParams->integer1);
	} else {
		// TODO: Check for AREA variables
		Core::Get()->Vars().Set(fActionParams->string1, fActionParams->integer1);
	}
	SetCompleted();
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


// ChangeArea / LeaveAreaLUA
ActionChangeArea::ActionChangeArea(Object* object, action_params* node)
	:
	Action(object, node)
{
}


/* virtual */
void
ActionChangeArea::operator()()
{
	Core::Get()->LoadArea(fActionParams->string1, "", "");
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
	IE::point point = fActionParams->where;
	if (point.x == -1 && point.y == -1) {
		Actor* thisActor = dynamic_cast<Actor*>(fSender);
		if (thisActor != NULL) {
			point = thisActor->Position();
			point.x += Core::RandomNumber(-20, 20);
			point.y += Core::RandomNumber(-20, 20);
		}
	}
#if 0
	std::cout << "create actor at " << point.x << ", " << point.y << std::endl;
#endif
	Actor* actor = new Actor(fActionParams->string1, point, fActionParams->integer1);
	((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);

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
	Actor* actor = new Actor(fActionParams->string1,
						fActionParams->where, fActionParams->integer1);
	std::cout << "Created actor (IMPASSABLE) " << fActionParams->string1 << " on ";
	std::cout << fActionParams->where.x << ", " << fActionParams->where.y << std::endl;
	//actor->SetDestination(fActionParams->where);
	((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);

	SetCompleted();
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
	Region* region = dynamic_cast<Region*>(Script::GetTargetObject(fSender, fActionParams));
	if (region != NULL)
		region->ActivateTrigger(fActionParams->integer1);
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
	Object* sender = Script::GetSenderObject(fSender, fActionParams);
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
	Object* object = Script::GetSenderObject(fSender, fActionParams);
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
	// TODO: Code duplication between here and ActionForceSpellPoint.
	// Refactor
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (sender == NULL) {
		std::cerr << "ActionForceSpell:: NO sender Actor" << std::endl;
		SetCompleted();
		return;
	}

	if (!Initiated()) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(fActionParams->integer1).c_str();
		std::string spellResourceName = SPLResource::GetSpellResourceName(fActionParams->integer1);
		gResManager->ReleaseResource(spellIDS);

		std::cout << "spell: " << spellName << std::endl;
		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		uint16 castTime = spellResource->CastingTime();

		// TODO: Not sure if it's correct. CastingTime is 1/10 of round.
		// Round takes 6 seconds (in BG)
		// AI update every 15 ticks (do we respect this ? don't know)
		fDuration = castTime * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "casting time:" << fDuration << std::endl;
		//std::cout << spellResource->DescriptionIdentifiedRef() << std::endl;
		//std::cout << spellResource->DescriptionUnidentifiedRef() << std::endl;
		gResManager->ReleaseResource(spellResource);
		sender->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		SetInitiated();
	}

	// TODO: only for testing
	if (fDuration-- == 0) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(fActionParams->integer1).c_str();
		gResManager->ReleaseResource(spellIDS);

		std::cout << "Spell " << spellName << " finished" << std::endl;
		sender->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		Object* target = Script::GetTargetObject(sender, fActionParams);
		if (target == NULL)
			target = sender;
		if (target != NULL) {
			std::cout << "target: " << target->Name() << std::endl;
			std::cout << "spell name: " << spellName << std::endl;
			SpellEffect* dummy = new SpellEffect(spellName);
			target->AddSpellEffect(dummy);
		}
		SetCompleted();
		std::cout << "duration:" << (Timer::Ticks() - fStart) << std::endl;
	}
}


// ForceSpellPoint
ActionForceSpellPoint::ActionForceSpellPoint(Object* object, action_params* node)
	:
	Action(object, node),
	fDuration(50)
{
	fStart = Timer::Ticks();
}


/* virtual */
void
ActionForceSpellPoint::operator()()
{
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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
		uint16 castTime = spellResource->CastingTime();

		// TODO: Not sure if it's correct. CastingTime is 1/10 of round.
		// Round takes 6 seconds (in BG)
		// AI update every 15 ticks (do we respect this ? don't know)
		fDuration = castTime * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "casting time:" << fDuration << std::endl;
		//std::cout << spellResource->DescriptionIdentifiedRef() << std::endl;
		//std::cout << spellResource->DescriptionUnidentifiedRef() << std::endl;
		gResManager->ReleaseResource(spellResource);
		sender->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		SetInitiated();
	}

	// TODO: only for testing
	if (fDuration-- == 0) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(fActionParams->integer1).c_str();
		gResManager->ReleaseResource(spellIDS);

		std::cout << "Spell " << spellName << " finished" << std::endl;
		sender->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		Object* target = Script::GetTargetObject(sender, fActionParams);
		if (target == NULL)
			target = sender;
		if (target != NULL) {
			std::cout << "target: " << target->Name() << std::endl;
			std::cout << "spell name: " << spellName << std::endl;
			SpellEffect* dummy = new SpellEffect(spellName);
			target->AddSpellEffect(dummy);
		}
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
		Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
		if (actor != NULL) {
			if (::strcasecmp(fActionParams->string1, actor->CurrentRegion()->Name()) != 0) {
				std::cerr << "BUG: ActionMoveBetweenAreasEffect() IMPLEMENT MOVING TO AREAS" << std::endl;
				Game::TempState* tempState = Game::Get()->GetTempState();
				actor->Acquire();
				tempState->actors.push_back(actor);
			} else {
				actor->SetPosition(fActionParams->where);
				actor->SetOrientation(fActionParams->integer1);
			}
		}
		SetCompleted();
	}
}


// PlayDeadAction
ActionPlayDead::ActionPlayDead(Object* object, action_params* node)
	:
	Action(object, node)
{
	fDuration = fActionParams->integer1;
}


/* virtual */
void
ActionPlayDead::operator()()
{
	if (!Initiated()) {
		SetInitiated();
		Actor* actor = dynamic_cast<Actor*>(fSender);
		if (actor == NULL) {
			SetCompleted();
			return;
		}
		actor->SetInterruptable(false);
		actor->SetAnimationAction(ACT_DEAD);
	}
	
	if (fDuration-- <= 0) {
		std::cout << "PlayDead finished" << std::endl;
		Actor* actor = dynamic_cast<Actor*>(fSender);
		actor->SetAnimationAction(ACT_STANDING);
		SetCompleted();
	}
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
	fSender->SetInterruptable(fActionParams->integer1 == 1);
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (!Initiated()) {	
		actor->SetDestination(fActionParams->where);
		SetInitiated();
	}

	actor->SetInterruptable(fInterruptable);

	if (!actor->MoveToNextPointInPath(false))
		SetCompleted();
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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

	if (!actor->MoveToNextPointInPath(false))
		SetCompleted();
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor == NULL)
		return;

	// TODO: Fly
	int16 randomX = Core::RandomNumber(-50, 50);
	int16 randomY = Core::RandomNumber(-50, 50);

	IE::point destination = offset_point(actor->Position(), randomX, randomY);
	if (!PointSufficientlyClose(actor->Position(), destination))
		actor->SetDestination(destination, true);

	if (actor->Position() == actor->Destination()) {
		//SetCompleted();
	} else {
		actor->MoveToNextPointInPath(true);
	}
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor == NULL)
		return;

	if (!Initiated()) {
		actor->SetDestination(fActionParams->where, true);
		SetInitiated();
	}

	if (actor->Position() == actor->Destination())
		SetCompleted();

	actor->MoveToNextPointInPath(true);
}


// ActionShout
ActionShout::ActionShout(Object* sender, action_params* params)
	:
	Action(sender, params)
{
}


/* virtual */
void
ActionShout::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor == NULL)
		return;

	actor->Shout(fActionParams->integer1);
}



// EscapeArea
ActionEscapeArea::ActionEscapeArea(Object* object, action_params* node)
:
	Action(object, node)
{
}


/* virtual */
void
ActionEscapeArea::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor == NULL)
		return;

	// TODO: destroying is a bit too much: escape area by walking or other
	actor->DestroySelf();
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor == NULL)
		return;

	if (!actor->IsWalking()) {
		int16 randomX = Core::RandomNumber(-50, 50);
		int16 randomY = Core::RandomNumber(-50, 50);

		IE::point destination = offset_point(actor->Position(), randomX, randomY);
		if (!PointSufficientlyClose(actor->Position(), destination))
			actor->SetDestination(destination);
	}
	if (actor->Position() == actor->Destination()) {
		//SetCompleted();
	} else {
		actor->MoveToNextPointInPath(true);
	}
}


// Wait
ActionWait::ActionWait(Object* object, action_params* node)
	:
	Action(object, node),
	fWaitTime(fActionParams->integer1 * AI_UPDATE_FREQ)
{
}


/* virtual */
void
ActionWait::operator()()
{
	if (--fWaitTime <= 0)
		SetCompleted();
}


// SmallWait
ActionSmallWait::ActionSmallWait(Object* object, action_params* node)
	:
	Action(object, node),
	fWaitTime(fActionParams->integer1)
{
}


/* virtual */
void
ActionSmallWait::operator()()
{
	//Object* object = Script::FindObject(fObject, fActionParams);
	//if (object != NULL)
	//	object->SetWaitTime(fActionParams->integer1);
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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


// ActionCloseDoor
ActionCloseDoor::ActionCloseDoor(Object* sender, action_params* node)
	:
	Action(sender, node)
{
}


/* virtual */
void
ActionCloseDoor::operator()()
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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

	std::cout << "actor " << actor->Name() << " closes " << door->Name() << std::endl;
	if (door->Opened()) {
		door->Close(actor);
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
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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
	if (actor->Area()->Distance(actor, target) < 200) {
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
}


/* virtual */
void
ActionDialog::operator()()
{
	Object* object = Script::GetSenderObject(fSender, fActionParams);
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
	Actor* actor = dynamic_cast<Actor*>(fSender);
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
#if 0
		std::cout << std::dec;
		std::cout << "offset: " << offset.x << ", " << offset.y << std::endl;
		std::cout << "fDestination: " << fDestination.x << ", " << fDestination.y << std::endl;
#endif
	} else
		SetCompleted();
}


ActionStartTimer::ActionStartTimer(Object* object, action_params* params)
	:
	Action(object, params)
{
}


/* virtual */
void
ActionStartTimer::operator()()
{
	// TODO: We use the id as part of the name
	std::ostringstream stringStream;
	stringStream << fSender->Name() << " " << fActionParams->integer1;
	GameTimer::Add(stringStream.str().c_str(), fActionParams->integer2 * AI_UPDATE_FREQ);
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
	if (!Initiated()) {
		SetInitiated();
		if (fSender != NULL)
			fSender->SetWaitTime(fDuration);
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
	fSender->Area()->ClearAllActions();
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
	GameTimer::Add(timerName.c_str(), fActionParams->integer1 * AI_UPDATE_FREQ);
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
		Object* sender = Script::GetSenderObject(fSender, fActionParams);
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
		Object* sender = Script::GetSenderObject(fSender, fActionParams);
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
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	if (actor != NULL) {
		actor->SetOrientation(fActionParams->integer1);
		actor->SetWaitTime(1);
	}
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
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
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
	sender->SetWaitTime(1);
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
	AreaRoom* area = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (area == NULL)
		return;

	Effect* effect = new Effect(fActionParams->string1, fActionParams->where);
	area->AddEffect(effect);
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
	Actor* sender = dynamic_cast<Actor*>(Script::GetSenderObject(fSender, fActionParams));
	Object* target = Script::GetTargetObject(sender, fActionParams);
	IE::point point;

	if (target->Type() == Object::ACTOR) {
		point = dynamic_cast<Actor*>(target)->Position();
	} else {
		point.x = target->Frame().x_max - target->Frame().x_min;
		point.y = target->Frame().y_max - target->Frame().y_min;
	}

	Effect* effect = new Effect(fActionParams->string1, point);
	sender->Area()->AddEffect(effect);
	SetCompleted();
}
