#include "Actions.h"

#include "Actor.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Effect.h"
#include "Game.h"
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "Object.h"
#include "Region.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Script.h"
#include "SpellEffect.h"
#include "SPLResource.h"
#include "Timer.h"
// TODO: Remove this dependency
#include "TLKResource.h"
#include "Variables.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>


static bool
PointSufficientlyClose(const IE::point& pointA, const IE::point& pointB)
{
	return (std::abs(pointA.x - pointB.x) <= 5 * 2)
		&& (std::abs(pointA.y - pointB.y) <= 5 * 2);
}


// ---- Native action implementations (proof of concept: one stateless, one
// with simple state, one with more involved state + resource access) ----

// SETGLOBAL(S:NAME*,S:AREA*,I:VALUE*) - stateless, completes immediately.
static void
RunActionSetGlobal(Object* sender, action_params* params, action_state& state)
{
	std::string variableScope;
	std::string variableName;
	Variables::GetNameAndScope(params->string1, variableScope, variableName);
	if (variableScope.compare("LOCALS") == 0) {
		Object* object = Script::GetSenderObject(sender, params);
		if (object != NULL)
			object->SetVariable(variableName.c_str(), params->integer1);
	} else {
		// TODO: Check for AREA variables
		Core::Get()->Vars().Set(params->string1, params->integer1);
	}
	state.completed = true;
}


// WAIT(I:TIME*) - simple state: a single tick countdown, reusing
// action_state::counter.
static void
RunActionWait(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.counter = params->integer1 * AI_UPDATE_FREQ;
		state.initiated = true;
	}
	if (--state.counter <= 0)
		state.completed = true;
}


// FORCESPELL(O:TARGET,I:SPELL*SPELL) - more involved state: resource
// lookups done once (on first tick), a tick countdown derived from the
// spell's casting time, and a start timestamp kept only for the diagnostic
// print at the end (mirrors the original ActionForceSpell::operator()()).
static void
RunActionForceSpell(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor == NULL) {
		std::cerr << "ForceSpell: NO sender Actor" << std::endl;
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		uint16 castTime = spellResource->CastingTime();
		// TODO: Not sure if it's correct. CastingTime is 1/10 of round.
		// Round takes ROUND_DURATION_SEC seconds; AI updates AI_UPDATE_FREQ
		// times per second.
		state.counter = castTime * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "casting time:" << state.counter << std::endl;
		gResManager->ReleaseResource(spellResource);

		actor->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		state.startTick = Timer::Ticks();
		state.initiated = true;
	}

	if (state.counter-- == 0) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		gResManager->ReleaseResource(spellIDS);
		std::cout << "Spell " << spellName << " finished" << std::endl;

		actor->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		Object* target = Script::GetTargetObject(sender, params);
		if (target == NULL)
			target = sender;
		if (target != NULL) {
			std::cout << "target: " << target->Name() << std::endl;
			std::cout << "spell name: " << spellName << std::endl;
			std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
			SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
			if (spellResource != NULL) {
				for (const spl_effect& effect : spellResource->Effects()) {
					target->AddSpellEffect(new SpellEffect(effect.opcode, sender,
						effect.parameter1, effect.parameter2, effect.duration,
						effect.resource.CString()));
				}
				gResManager->ReleaseResource(spellResource);
			}
		}
		state.completed = true;
		std::cout << "duration:" << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// CREATECREATURE(S:NewObject*,P:Location*,I:Face*) - stateless.
// TODO: If point is (-1, -1) we should put the actor near the active
// creature. Which one is the active creature?
static void
RunActionCreateCreature(Object* sender, action_params* params, action_state& state)
{
	IE::point point = params->where;
	if (point.x == -1 && point.y == -1) {
		Actor* thisActor = dynamic_cast<Actor*>(sender);
		if (thisActor != NULL) {
			point = thisActor->Position();
			point.x += Core::RandomNumber(-20, 20);
			point.y += Core::RandomNumber(-20, 20);
		}
	}
	Actor* actor = new Actor(params->string1, point, params->integer1);
	((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	state.completed = true;
}


// CREATECREATUREIMPASSABLE(S:NewObject*,P:Location*,I:Face*) - stateless.
static void
RunActionCreateCreatureImpassable(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = new Actor(params->string1, params->where, params->integer1);
	std::cout << "Created actor (IMPASSABLE) " << params->string1 << " on ";
	std::cout << params->where.x << ", " << params->where.y << std::endl;
	((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	state.completed = true;
}


// TRIGGERACTIVATION(O:OBJECT*,I:STATE*BOOLEAN) - stateless.
static void
RunActionTriggerActivation(Object* sender, action_params* params, action_state& state)
{
	Region* region = dynamic_cast<Region*>(Script::GetTargetObject(sender, params));
	if (region != NULL)
		region->ActivateTrigger(params->integer1);
	state.completed = true;
}


// UNLOCK(O:OBJECT*) - stateless.
static void
RunActionUnlock(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	Door* door = dynamic_cast<Door*>(target);
	if (door == NULL) {
		std::cerr << "NULL DOOR!!! MEANS THE OBJECT IS NOT A DOOR" << std::endl;
		state.completed = true;
		return;
	}
	door->Unlock();
	state.completed = true;
}


// DESTROYSELF() - stateless.
static void
RunActionDestroySelf(Object* sender, action_params* params, action_state& state)
{
	// Re-resolve at execution time rather than using `sender` (the object
	// whose queue this happens to run from) directly: this action's real
	// target may have been created by an earlier action in the same
	// cutscene block, which hadn't executed yet (only been queued) when
	// this action's sender was first resolved at queue time.
	Object* object = Script::GetSenderObject(sender, params);
	if (object != NULL)
		object->DestroySelf();
	state.completed = true;
}


// FORCESPELLPOINT(P:TARGET,I:SPELL*SPELL) - same shape as RunActionForceSpell,
// just targeting a point instead of the sender's current target object.
static void
RunActionForceSpellPoint(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor == NULL) {
		std::cerr << "ForceSpellPoint: NO sender Actor" << std::endl;
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		uint16 castTime = spellResource->CastingTime();
		state.counter = castTime * AI_UPDATE_FREQ * ROUND_DURATION_SEC / 10;
		std::cout << "casting time:" << state.counter << std::endl;
		gResManager->ReleaseResource(spellResource);

		actor->SetAnimationAction(ACT_CAST_SPELL_PREPARE);
		state.startTick = Timer::Ticks();
		state.initiated = true;
	}

	if (state.counter-- == 0) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		gResManager->ReleaseResource(spellIDS);
		std::cout << "Spell " << spellName << " finished" << std::endl;

		actor->SetAnimationAction(ACT_CAST_SPELL_RELEASE);
		Object* target = Script::GetTargetObject(sender, params);
		if (target == NULL)
			target = sender;
		if (target != NULL) {
			std::cout << "target: " << target->Name() << std::endl;
			std::cout << "spell name: " << spellName << std::endl;
			std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
			SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
			if (spellResource != NULL) {
				for (const spl_effect& effect : spellResource->Effects()) {
					target->AddSpellEffect(new SpellEffect(effect.opcode, sender,
						effect.parameter1, effect.parameter2, effect.duration,
						effect.resource.CString()));
				}
				gResManager->ReleaseResource(spellResource);
			}
		}
		state.completed = true;
		std::cout << "duration:" << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// MOVEBETWEENAREASEFFECT(S:AREA*,S:EFFECT*,P:LOCATION*,I:FACE*) - resolves
// and completes in a single tick (mirrors the original, which never left
// state.initiated false for more than one call).
static void
RunActionMoveBetweenAreasEffect(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		Actor* actor = dynamic_cast<Actor*>(sender);
		if (actor != NULL) {
			if (::strcasecmp(params->string1, actor->Area()->Name()) != 0) {
				std::cerr << "BUG: MoveBetweenAreasEffect() IMPLEMENT MOVING TO AREAS" << std::endl;
				Game::TempState* tempState = Game::Get()->GetTempState();
				actor->Acquire();
				tempState->actors.push_back(actor);
			} else {
				actor->SetPosition(params->where);
				actor->SetOrientation(params->integer1);
			}
		}
		state.completed = true;
	}
}


// PLAYDEAD(I:Time*) - measured in AI updates per second.
static void
RunActionPlayDead(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		// Unlike Wait() (seconds), PlayDead's Time* is already in AI
		// updates (see IESDP and SmallWait's identical unit)
		state.counter = params->integer1;
		state.initiated = true;
		actor->SetInterruptable(false);
		actor->SetAnimationAction(ACT_DEAD);
	}

	if (state.counter-- <= 0) {
		std::cout << "PlayDead finished" << std::endl;
		actor->SetAnimationAction(ACT_STANDING);
		state.completed = true;
	}
}


// SETINTERRUPT(I:State*Boolean) - stateless.
static void
RunActionSetInterruptable(Object* sender, action_params* params, action_state& state)
{
	Object* object = Script::GetSenderObject(sender, params);
	if (object != NULL)
		object->SetInterruptable(params->integer1 == 1);
	state.completed = true;
}


// MOVETOPOINT(P:Point*) / MOVETOPOINTNOINTERRUPT(P:Point*) - same run
// function for both ids; whether the walk can be interrupted is decided by
// which id was used to queue it (207 = no-interrupt), not by any parameter,
// so it's derived fresh from params->id every tick rather than stored.
static void
RunActionWalkTo(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		actor->SetDestination(params->where);
		state.initiated = true;
	}

	bool canInterrupt = params->id != 207; // 207 = MOVETOPOINTNOINTERRUPT
	actor->SetInterruptable(canInterrupt);

	if (!actor->MoveToNextPointInPath(false))
		state.completed = true;
}


// MOVETOOBJECT(O:Target*) - no persistent state: the destination is
// recomputed every tick since the target may be moving.
static void
RunActionWalkToObject(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	Object* target = Script::GetTargetObject(actor, params);
	if (target == NULL) {
		state.completed = true;
		return;
	}

	IE::point destination = target->NearestPoint(actor->Position());
	if (!PointSufficientlyClose(actor->Position(), destination))
		actor->SetDestination(destination);

	if (!actor->MoveToNextPointInPath(false))
		state.completed = true;
}


// RANDOMFLY() - per IESDP, mirrors RANDOMWALK (gives the appearance of
// flying by passing over impassable terrain) and completes once the
// randomly-chosen point is reached, the same as RANDOMWALK below - not
// "never", which would stall any action queued after it (e.g. IESDP's own
// RandomWalk() example: "RandomWalk(); Wait(5); RandomWalk();"). Also
// mirrors RANDOMWALK's `if (!actor->IsWalking())` guard, so a fresh random
// point isn't rolled every single tick while a walk to the previous one is
// still in progress.
static void
RunActionRandomFly(Object* sender, action_params* params, action_state& state)
{
	// TODO: We should fly in straight line
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!actor->IsWalking()) {
		IE::point randomValue = {
			int16(Core::RandomNumber(-50, 50)),
			int16(Core::RandomNumber(-50, 50))
		};
		IE::point destination = actor->Position() + randomValue;
		if (!PointSufficientlyClose(actor->Position(), destination))
			actor->SetDestination(destination, true);
	}

	if (actor->Position() == actor->Destination())
		state.completed = true;
	else
		actor->MoveToNextPointInPath(true);
}


// FLYTOPOINT(P:Point*, I:time*) - id 101. Per IESDP, "used internally by
// action 100 (RandomFly); it moves the active creature towards the given
// point for the specified amount of time" - i.e. it gives up once that
// time (in AI updates, same unit as SmallWait/PlayDead/RunAwayFrom) elapses,
// not only on arrival.
static void
RunActionFlyTo(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		actor->SetDestination(params->where, true);
		state.counter = params->integer1;
		state.initiated = true;
	}

	if (actor->Position() == actor->Destination() || state.counter-- <= 0) {
		state.completed = true;
		return;
	}

	actor->MoveToNextPointInPath(true);
}


// SHOUT(I:Number*) - stateless.
static void
RunActionShout(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	actor->Shout(params->integer1);
	state.completed = true;
}


// ESCAPEAREA()/ESCAPEAREAMOVE() - stateless.
// TODO: destroying is a bit too much: escape area by walking or other means.
static void
RunActionEscapeArea(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	actor->DestroySelf();
	state.completed = true;
}


// INCREMENTGLOBAL(S:NAME*,S:AREA*,I:VALUE*) - stateless.
static void
RunActionIncrementGlobal(Object* sender, action_params* params, action_state& state)
{
	Core* core = Core::Get();
	int32 value = core->Vars().Get(params->string1);
	core->Vars().Set(params->string1, value + params->integer1);
	state.completed = true;
}


// LEAVEAREALUA(S:Area*,S:Parchment*,P:Point*,I:Face*) - stateless.
static void
RunActionChangeArea(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->LoadArea(params->string1, "", "");
	state.completed = true;
}


// RANDOMWALK() - per IESDP, this completes (like any other action) once
// the randomly-chosen point is reached; IESDP's own example script queues
// "RandomWalk(); Wait(5); RandomWalk();" in sequence, which would stall
// forever after the first call if this never completed.
static void
RunActionRandomWalk(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!actor->IsWalking()) {
		IE::point randomValue = {
			int16(Core::RandomNumber(-50, 50)),
			int16(Core::RandomNumber(-50, 50))
		};
		IE::point destination = actor->Position() + randomValue;
		if (!PointSufficientlyClose(actor->Position(), destination))
			actor->SetDestination(destination);
	}
	if (actor->Position() == actor->Destination())
		state.completed = true;
	else
		actor->MoveToNextPointInPath(true);
}


// SMALLWAIT(I:Time*) - unlike WAIT, not scaled by AI_UPDATE_FREQ.
static void
RunActionSmallWait(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.counter = params->integer1;
		state.initiated = true;
	}
	if (--state.counter <= 0)
		state.completed = true;
}


// OPENDOOR(O:OBJECT*) - stateless.
static void
RunActionOpenDoor(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		std::cerr << "NULL ACTOR!!!" << std::endl;
		state.completed = true;
		return;
	}

	Object* target = Script::GetTargetObject(actor, params);
	Door* door = dynamic_cast<Door*>(target);
	if (door == NULL) {
		std::cerr << "NULL DOOR!!! MEANS THE OBJECT IS NOT A DOOR" << std::endl;
		state.completed = true;
		return;
	}

	std::cout << "actor " << actor->Name() << " opens " << door->Name() << std::endl;
	if (!door->Opened()) {
		door->Open(actor);
		state.completed = true;
	}
}


// CLOSEDOOR(O:OBJECT*) - stateless.
static void
RunActionCloseDoor(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		std::cerr << "NULL ACTOR!!!" << std::endl;
		state.completed = true;
		return;
	}

	Object* target = Script::GetTargetObject(actor, params);
	Door* door = dynamic_cast<Door*>(target);
	if (door == NULL) {
		std::cerr << "NULL DOOR!!! MEANS THE OBJECT IS NOT A DOOR" << std::endl;
		state.completed = true;
		return;
	}

	std::cout << "actor " << actor->Name() << " closes " << door->Name() << std::endl;
	if (door->Opened()) {
		door->Close(actor);
		state.completed = true;
	}
}


// DISPLAYSTRING(O:Object*,I:StrRef*) - stateless. NOTE: this id (151) is
// currently wired to this class (originally ActionDisplayMessage), NOT to
// ActionDisplayString, which is never constructed anywhere in the legacy
// switch - it looks orphaned/unreachable in the current code, so it wasn't
// converted. Flagging in case DISPLAYSTRING(151) was meant to use it
// instead (ActionDisplayString honors position/duration via
// GUI::DisplayString(); this one always logs to the message window).
static void
RunActionDisplayMessage(Object* sender, action_params* params, action_state& state)
{
	std::cout << "DisplayMessage:: ";
	std::string dialogText = IDTable::GetDialog(params->integer1);
	std::cout << dialogText << std::endl;
	Core::Get()->DisplayMessage(NULL, dialogText.c_str());
	state.completed = true;
}


// STARTMOVIE(S:Movie*) - stateless.
static void
RunActionPlayMovie(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->PlayMovie(params->string1);
	state.completed = true;
}


// ATTACK(O:Target*) - per IESDP, continually attacks the target - it
// doesn't complete on its own until the target is dead.
// ATTACKREEVALUATE(O:Target*,I:ReevaluationPeriod*) - same, but only for up
// to ReevaluationPeriod AI updates (default 15/sec); once that elapses the
// action completes - even if the target is still alive - so the script
// re-runs and checks its other conditions, per IESDP. Only id 134 carries
// a period; plain ATTACK (id 3) has no time limit.
static void
RunActionAttack(Object* sender, action_params* params, action_state& state)
{
	Actor* actorSender = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actorSender == NULL) {
		state.completed = true;
		return;
	}

	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(actorSender, params));
	if (target == NULL || target->IsState(STATE_DEAD)) {
		state.completed = true;
		return;
	}

	if (params->id == 134) { // ATTACKREEVALUATE
		if (!state.initiated) {
			state.counter = params->integer1;
			state.initiated = true;
		}
		if (state.counter-- <= 0) {
			state.completed = true;
			return;
		}
	}

	IE::point point = target->NearestPoint(actorSender->Position());
	if (!PointSufficientlyClose(actorSender->Position(), point))
		actorSender->SetDestination(point);

	if (actorSender->Position() != actorSender->Destination()) {
		actorSender->SetAnimationAction(ACT_WALKING);
		actorSender->MoveToNextPointInPath(actorSender->IsFlying());
	} else {
		actorSender->SetAnimationAction(ACT_ATTACKING);
		// Paced by AttackCooldown() rather than resolving a hit every
		// single tick: state.counter is already claimed above by
		// ATTACKREEVALUATE's own reevaluation-period countdown, so the
		// per-round cooldown lives on the Actor itself instead.
		if (actorSender->AttackCooldown() > 0) {
			actorSender->SetAttackCooldown(actorSender->AttackCooldown() - 1);
		} else {
			actorSender->AttackTarget(target);
			uint8 attacksPerRound = actorSender->CRE()->NumberOfAttacks();
			if (attacksPerRound == 0)
				attacksPerRound = 1;
			actorSender->SetAttackCooldown(
					(AI_UPDATE_FREQ * ROUND_DURATION_SEC) / attacksPerRound);
		}
	}
}


static IE::point
PointAway(Actor* actor, Actor* target)
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


// RUNAWAYFROM(O:Creature*,I:Time*) - per IESDP, flees from the target for
// the specified time (in AI updates), not just until some fixed distance
// is reached - the target's own point-away logic below (still a TODO to
// improve) keeps recomputing every tick, but the action as a whole only
// completes once the time elapses.
static void
RunActionRunAwayFrom(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(actor, params));
	if (target == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		state.counter = params->integer1;
		state.initiated = true;
	}

	if (state.counter-- <= 0) {
		state.completed = true;
		actor->SetAnimationAction(ACT_STANDING);
		return;
	}

	// TODO: Improve implementation
	if (actor->Area()->Distance(actor, target) < 200) {
		IE::point point = PointAway(actor, target);
		if (actor->Destination() != point)
			actor->SetDestination(point);
	}

	if (actor->Position() == actor->Destination()) {
		actor->SetAnimationAction(ACT_STANDING);
	} else {
		actor->SetAnimationAction(ACT_WALKING);
		actor->MoveToNextPointInPath(actor->IsFlying());
	}
}


// DIALOGUE(O:OBJECT*) / STARTDIALOGNOSET(O:OBJECT*) - same run function for
// both ids.
// TODO: Some dialogue actions require the actor to be near the target,
// others do not. Must be able to differentiate (see commented-out original).
static void
RunActionDialog(Object* sender, action_params* params, action_state& state)
{
	Object* object = Script::GetSenderObject(sender, params);
	if (object == NULL) {
		state.completed = true;
		return;
	}

	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(object, params));
	if (target == NULL || target->IsState(STATE_DEAD)) {
		state.completed = true;
		return;
	}

	Actor* speaker = dynamic_cast<Actor*>(object);
	if (speaker != NULL && speaker->IsState(STATE_DEAD)) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		if (speaker != NULL)
			Game::Get()->InitiateDialog(speaker, target);
		state.initiated = true;
	}
	std::cout << "Actor " << object->Name();
	std::cout << " will talk to " << target->Name() << std::endl;
	state.completed = true;
}


// ENEMY() - stateless.
static void
RunActionSetEnemyAlly(Object* sender, action_params* params, action_state& state)
{
	uint32 id = IDTable::EnemyAllyValue("ENEM");
	// TODO: Correct ? or should we get the sender object ?
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor != NULL)
		actor->SetEnemyAlly(id);
	state.completed = true;
}


// FADETOCOLOR(P:POINT*,I:BLUE*) - state.counter/extra/step map to the
// original's fCurrentValue/fTargetValue/fStepValue.
static void
RunActionFadeToColor(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.counter = 255;   // current
		state.extra = 0;       // target
		state.step = (state.counter - state.extra) / params->where.x;
	}

	GraphicsEngine::Get()->SetFade(state.counter);
	if (state.counter > state.extra)
		state.counter -= state.step;
	else
		state.completed = true;
}


// FADEFROMCOLOR(P:POINT*,I:BLUE*)
static void
RunActionFadeFromColor(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.counter = 0;     // current
		state.extra = 255;     // target
		state.step = state.extra / params->where.x;
	}

	GraphicsEngine::Get()->SetFade(state.counter);
	if (state.counter < state.extra)
		state.counter += state.step;
	else
		state.completed = true;
}


// MOVEVIEWPOINT(P:TARGET*,I:SCROLLSPEED*SCROLL) - state.point/extra map to
// the original's fDestination/fScrollSpeed.
static void
RunActionMoveViewPoint(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.point = params->where;
		Core::Get()->CurrentRoom()->SanitizeOffsetCenter(state.point);
		switch (params->integer1) {
			case 1:
				state.extra = 10;
				break;
			case 2:
				state.extra = 20;
				break;
			case 3:
				state.extra = 40;
				break;
			case 4:
				state.extra = 80;
				break;
			case 0:
			default:
				state.extra = 10000;
				break;
		}
	}

	RoomBase* room = Core::Get()->CurrentRoom();
	IE::point offset = room->AreaCenterPoint();
	const int16 step = state.extra;
	if (offset != state.point) {
		if (offset.x > state.point.x)
			offset.x = std::max((int16)(offset.x - step), state.point.x);
		else if (offset.x < state.point.x)
			offset.x = std::min((int16)(offset.x + step), state.point.x);

		if (offset.y > state.point.y)
			offset.y = std::max((int16)(offset.y - step), state.point.y);
		else if (offset.y < state.point.y)
			offset.y = std::min((int16)(offset.y + step), state.point.y);
		room->SetAreaOffsetCenter(offset);
	} else
		state.completed = true;
}


// STARTTIMER(I:ID*,I:Time*) - stateless.
static void
RunActionStartTimer(Object* sender, action_params* params, action_state& state)
{
	// TODO: We use the id as part of the name
	std::ostringstream stringStream;
	stringStream << sender->Name() << " " << params->integer1;
	GameTimer::Add(stringStream.str().c_str(), params->integer2 * AI_UPDATE_FREQ);
	state.completed = true;
}


// SCREENSHAKE(P:POINT*,I:DURATION*) - state.counter/point map to the
// original's fDuration/fOffset.
static void
RunActionScreenShake(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.counter = params->integer1;
		if (sender != NULL)
			sender->SetWaitTime(state.counter);
		state.point = params->where;
	}

	GFX::point point = { 0, 0 };
	if (state.counter-- == 0) {
		GraphicsEngine::Get()->SetRenderingOffset(point);
		state.completed = true;
		return;
	}

	point.x = state.point.x;
	point.y = state.point.y;

	GraphicsEngine::Get()->SetRenderingOffset(point);
	state.point.x = -state.point.x;
	state.point.y = -state.point.y;
}


// STARTCUTSCENEMODE() - stateless.
static void
RunActionStartCutsceneMode(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->StartCutsceneMode();
	state.completed = true;
}


// ENDCUTSCENEMODE() - stateless.
static void
RunActionEndCutsceneMode(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->EndCutsceneMode();
	state.completed = true;
}


// CLEARALLACTIONS() - stateless.
static void
RunActionClearAllActions(Object* sender, action_params* params, action_state& state)
{
	sender->Area()->ClearAllActions();
	state.completed = true;
}


// SETGLOBALTIMER(S:NAME*,S:AREA*,I:TIME*GTIMES) - stateless.
static void
RunActionSetGlobalTimer(Object* sender, action_params* params, action_state& state)
{
	std::string timerName;
	// TODO: We append the timer name to the area name, check if it's okay
	timerName.append(params->string2).append(params->string1);
	GameTimer::Add(timerName.c_str(), params->integer1 * AI_UPDATE_FREQ);
	state.completed = true;
}


// STARTCUTSCENE(S:CUTSCENE*) - stateless.
static void
RunActionStartCutscene(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->StartCutscene(params->string1);
	state.completed = true;
}


// HIDEGUI() - stateless.
static void
RunActionHideGUI(Object* sender, action_params* params, action_state& state)
{
	GUI::Get()->Hide();
	state.completed = true;
}


// UNHIDEGUI() - stateless.
static void
RunActionUnhideGUI(Object* sender, action_params* params, action_state& state)
{
	GUI::Get()->Show();
	state.completed = true;
}


// DISPLAYSTRINGHEAD(O:OBJECT*,I:STRREF*) / DISPLAYSTRINGWAIT(...) - same
// run function for both ids. state.counter mirrors the original's fDuration
// (hardcoded to 100, marked "??" in the source it was ported from).
static void
RunActionDisplayStringHead(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.counter = 100; // ??
		Object* resolvedSender = Script::GetSenderObject(sender, params);
		Actor* actor = dynamic_cast<Actor*>(Script::GetTargetObject(resolvedSender, params));
		if (actor == NULL) {
			std::cerr << "DisplayStringHead: no TARGET!!!" << std::endl;
			state.completed = true;
			return;
		}
		TLKEntry* tlkEntry = IDTable::GetTLKEntry(params->integer1);
		actor->SetText(tlkEntry->text);
		delete tlkEntry;
	}
	if (state.counter-- <= 0) {
		Object* resolvedSender = Script::GetSenderObject(sender, params);
		Actor* actor = dynamic_cast<Actor*>(Script::GetTargetObject(resolvedSender, params));
		if (actor != NULL)
			actor->SetText("");
		state.completed = true;
	}
}


// FACE(I:DIRECTION) - stateless.
static void
RunActionChangeOrientationExt(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		actor->SetOrientation(params->integer1);
		actor->SetWaitTime(1);
	}
	state.completed = true;
}


// FACEOBJECT(O:OBJECT*) - stateless.
static void
RunActionFaceObject(Object* sender, action_params* params, action_state& state)
{
	Actor* actorSender = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Object* target = Script::GetTargetObject(actorSender, params);
	if (actorSender == NULL || target == NULL) {
		std::cerr << "FaceObject(): NULL object" << std::endl;
		state.completed = true;
		return;
	}

	const IE::rect objectFrame = target->Frame();
	IE::point point;
	point.x = objectFrame.Width() / 2;
	point.y = objectFrame.Height() / 2;
	actorSender->SetOrientation(point);
	actorSender->SetWaitTime(1);
	state.completed = true;
}


// CREATEVISUALEFFECT(S:Object*,P:Location*) - stateless.
static void
RunActionCreateVisualEffect(Object* sender, action_params* params, action_state& state)
{
	AreaRoom* area = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (area == NULL)
		return;

	Effect* effect = new Effect(params->string1, params->where);
	area->AddEffect(effect);
	state.completed = true;
}


// CREATEVISUALEFFECTOBJECT(S:DIALOGFILE*,O:TARGET*) - stateless.
static void
RunActionCreateVisualEffectObject(Object* sender, action_params* params, action_state& state)
{
	Actor* actorSender = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actorSender == NULL) {
		state.completed = true;
		return;
	}

	Object* target = Script::GetTargetObject(actorSender, params);
	if (target == NULL) {
		state.completed = true;
		return;
	}

	IE::point point;

	if (target->Type() == Object::ACTOR) {
		point = dynamic_cast<Actor*>(target)->Position();
	} else {
		point.x = target->Frame().x_max - target->Frame().x_min;
		point.y = target->Frame().y_max - target->Frame().y_min;
	}

	Effect* effect = new Effect(params->string1, point);
	actorSender->Area()->AddEffect(effect);
	state.completed = true;
}



static const ActionDescriptor kActionsTable[] = {
		{ 0, "NOACTION", NULL },
		{ 1, "ACTIONOVERRIDE", NULL },
		{ 2, "ADDWAYPOINT", NULL },
		{ 3, "ATTACK", RunActionAttack },
		{ 5, "BACKSTAB", NULL },
		{ 7, "CREATECREATURE", RunActionCreateCreature },
		{ 8, "DIALOG", RunActionDialog },
		{ 9, "DROPITEM", NULL },
		{ 10, "ENEMY", RunActionSetEnemyAlly },
		{ 11, "EQUIPITEM", NULL },
		{ 13, "FINDTRAPS", NULL },
		{ 14, "GETITEM", NULL },
		{ 15, "GIVEITEM", NULL },
		{ 16, "GIVEORDER", NULL },
		{ 17, "HELP", NULL },
		{ 18, "HIDE", NULL },
		{ 19, "JOINPARTY", NULL },
		{ 20, "LAYHANDS", NULL },
		{ 21, "LEAVEPARTY", NULL },
		{ 22, "MOVETOOBJECT", RunActionWalkToObject },
		{ 23, "MOVETOPOINT", RunActionWalkTo },
		{ 24, "PANIC", NULL },
		{ 25, "PICKPOCKETS", NULL },
		{ 26, "PLAYSOUND", NULL },
		{ 27, "PROTECTPOINT", NULL },
		{ 28, "REMOVETRAPS", NULL },
		{ 29, "RUNAWAYFROM", RunActionRunAwayFrom },
		{ 30, "SETGLOBAL", RunActionSetGlobal },
		{ 31, "SPELL", NULL },
		{ 33, "TURN", NULL },
		{ 34, "USEITEMSLOT", NULL },
		{ 36, "CONTINUE", NULL },
		{ 37, "FOLLOWPATH", NULL },
		{ 38, "SWING", NULL },
		{ 39, "RECOIL", NULL },
		{ 40, "PLAYDEAD", RunActionPlayDead },
		{ 47, "FORMATION", NULL },
		{ 48, "JUMPTOPOINT", NULL },
		{ 49, "MOVEVIEWPOINT", RunActionMoveViewPoint },
		{ 50, "MOVEVIEWOBJECT", NULL },
		{ 51, "CLICKLBUTTONPOINT", NULL },
		{ 52, "CLICKLBUTTONOBJECT", NULL },
		{ 53, "CLICKRBUTTONPOINT", NULL },
		{ 54, "CLICKRBUTTONOBJECT", NULL },
		{ 55, "DOUBLECLICKLBUTTONPOINT", NULL },
		{ 56, "DOUBLECLICKLBUTTONOBJECT", NULL },
		{ 57, "DOUBLECLICKRBUTTONPOINT", NULL },
		{ 58, "DOUBLECLICKRBUTTONOBJECT", NULL },
		{ 59, "MOVECURSORPOINT", NULL },
		{ 60, "CHANGEAISCRIPT", NULL },
		{ 61, "STARTTIMER", RunActionStartTimer },
		{ 62, "SENDTRIGGER", NULL },
		{ 63, "WAIT", RunActionWait },
		{ 64, "UNDOEXPLORE", NULL },
		{ 65, "EXPLORE", NULL },
		{ 66, "DAYNIGHT", NULL },
		{ 67, "WEATHER", NULL },
		{ 68, "CALLLIGHTNING", NULL },
		{ 69, "VEQUIP", NULL },
		{ 70, "NIDSPECIAL1", NULL },
		{ 71, "NIDSPECIAL2", NULL },
		{ 72, "NIDSPECIAL3", NULL },
		{ 73, "NIDSPECIAL4", NULL },
		{ 74, "NIDSPECIAL5", NULL },
		{ 75, "NIDSPECIAL6", NULL },
		{ 76, "NIDSPECIAL7", NULL },
		{ 77, "NIDSPECIAL8", NULL },
		{ 78, "NIDSPECIAL9", NULL },
		{ 79, "NIDSPECIAL10", NULL },
		{ 80, "NIDSPECIAL11", NULL },
		{ 81, "NIDSPECIAL12", NULL },
		{ 82, "CREATEITEM", NULL },
		{ 83, "SMALLWAIT", RunActionSmallWait },
		{ 84, "FACE", RunActionChangeOrientationExt },
		{ 85, "RANDOMWALK", RunActionRandomWalk },
		{ 86, "SETINTERRUPT", RunActionSetInterruptable },
		{ 87, "PROTECTOBJECT", NULL },
		{ 88, "LEADER", NULL },
		{ 89, "FOLLOW", NULL },
		{ 90, "MOVETOPOINTNORECTICLE", NULL },
		{ 91, "LEAVEAREA", NULL },
		{ 92, "SELECTWEAPONABILITY", NULL },
		{ 94, "GROUPATTACK", NULL },
		{ 95, "SPELLPOINT", NULL },
		{ 96, "REST", NULL },
		{ 97, "USEITEMPOINTSLOT", NULL },
		{ 98, "ATTACKNOSOUND", NULL },
		{ 100, "RANDOMFLY", RunActionRandomFly },
		{ 101, "FLYTOPOINT", RunActionFlyTo }, // not in the original IDS table
		{ 102, "MORALESET", NULL },
		{ 103, "MORALEINC", NULL },
		{ 104, "MORALEDEC", NULL },
		{ 105, "ATTACKONEROUND", NULL },
		{ 106, "SHOUT", RunActionShout },
		{ 107, "MOVETOOFFSET", NULL },
		{ 108, "ESCAPEAREA", RunActionEscapeArea },
		{ 108, "ESCAPEAREAMOVE", RunActionEscapeArea },
		{ 109, "INCREMENTGLOBAL", RunActionIncrementGlobal },
		{ 110, "LEAVEAREALUA", RunActionChangeArea },
		{ 111, "DESTROYSELF", RunActionDestroySelf },
		{ 112, "USECONTAINER", NULL },
		{ 113, "FORCESPELL", RunActionForceSpell },
		{ 114, "FORCESPELLPOINT", RunActionForceSpellPoint },
		{ 115, "SETGLOBALTIMER", RunActionSetGlobalTimer },
		{ 116, "TAKEPARTYITEM", NULL },
		{ 117, "TAKEPARTYGOLD", NULL },
		{ 118, "GIVEPARTYGOLD", NULL },
		{ 119, "DROPINVENTORY", NULL },
		{ 120, "STARTCUTSCENE", RunActionStartCutscene },
		{ 121, "STARTCUTSCENEMODE", RunActionStartCutsceneMode },
		{ 122, "ENDCUTSCENEMODE", RunActionEndCutsceneMode },
		{ 123, "CLEARALLACTIONS", RunActionClearAllActions },
		{ 124, "BERSERK", NULL },
		{ 125, "DEACTIVATE", NULL },
		{ 126, "ACTIVATE", NULL },
		{ 127, "CUTSCENEID", NULL },
		{ 128, "ANKHEGEMERGE", NULL },
		{ 129, "ANKHEGHIDE", NULL },
		{ 130, "RANDOMTURN", NULL },
		{ 131, "KILL", NULL },
		{ 132, "VERBALCONSTANT", NULL },
		{ 133, "CLEARACTIONS", NULL },
		{ 134, "ATTACKREEVALUATE", RunActionAttack },
		{ 135, "LOCKSCROLL", NULL },
		{ 136, "UNLOCKSCROLL", NULL },
		{ 137, "STARTDIALOGUE", NULL },
		{ 138, "SETDIALOGUE", NULL },
		{ 139, "PLAYERDIALOGUE", NULL },
		{ 140, "GIVEITEMCREATE", NULL },
		{ 141, "GIVEPARTYGOLDGLOBAL", NULL },
		{ 142, "USEDOOR", NULL },
		{ 143, "OPENDOOR", RunActionOpenDoor },
		{ 144, "CLOSEDOOR", RunActionCloseDoor },
		{ 145, "PICKLOCK", NULL },
		{ 146, "POLYMORPH", NULL },
		{ 147, "REMOVESPELL", NULL },
		{ 148, "BASHDOOR", NULL },
		{ 149, "EQUIPMOSTDAMAGINGMELEE", NULL },
		{ 150, "STARTSTORE", NULL },
		{ 151, "DISPLAYSTRING", RunActionDisplayMessage },
		{ 152, "CHANGEAITYPE", NULL },
		{ 153, "CHANGEENEMYALLY", NULL },
		{ 154, "CHANGEGENERAL", NULL },
		{ 155, "CHANGERACE", NULL },
		{ 156, "CHANGECLASS", NULL },
		{ 157, "CHANGESPECIFICS", NULL },
		{ 158, "CHANGEGENDER", NULL },
		{ 159, "CHANGEALIGNMENT", NULL },
		{ 160, "APPLYSPELL", NULL },
		{ 161, "INCREMENTCHAPTER", NULL },
		{ 162, "REPUTATIONSET", NULL },
		{ 163, "REPUTATIONINC", NULL },
		{ 164, "ADDEXPERIENCEPARTY", NULL },
		{ 165, "ADDEXPERIENCEPARTYGLOBAL", NULL },
		{ 166, "SETNUMTIMESTALKEDTO", NULL },
		{ 167, "STARTMOVIE", RunActionPlayMovie },
		{ 168, "INTERACT", NULL },
		{ 169, "DESTROYITEM", NULL },
		{ 170, "REVEALAREAONMAP", NULL },
		{ 171, "GIVEGOLDFORCE", NULL },
		{ 172, "CHANGETILESTATE", NULL },
		{ 173, "ADDJOURNALENTRY", NULL },
		{ 174, "EQUIPRANGED", NULL },
		{ 175, "SETLEAVEPARTYDIALOGUEFILE", NULL },
		{ 176, "ESCAPEAREADESTROY", NULL },
		{ 177, "TRIGGERACTIVATION", RunActionTriggerActivation },
		{ 178, "BREAKINSTANTS", NULL },
		{ 179, "DIALOGUEINTERRUPT", NULL },
		{ 180, "MOVETOOBJECTFOLLOW", NULL },
		{ 181, "REALLYFORCESPELL", NULL },
		{ 182, "MAKEUNSELECTABLE", NULL },
		{ 183, "MULTIPLAYERSYNC", NULL },
		{ 184, "RUNAWAYFROMNOINTERRUPT", NULL },
		{ 185, "SETMASTERAREA", NULL },
		{ 186, "ENDCREDITS", NULL },
		{ 187, "STARTMUSIC", NULL },
		{ 188, "TAKEPARTYITEMALL", NULL },
		{ 189, "LEAVEAREALUAPANIC", NULL },
		{ 190, "SAVEGAME", NULL },
		{ 191, "SPELLNODEC", NULL },
		{ 192, "SPELLPOINTNODEC", NULL },
		{ 193, "TAKEPARTYITEMRANGE", NULL },
		{ 194, "CHANGEANIMATION", NULL },
		{ 195, "LOCK", NULL },
		{ 196, "UNLOCK", RunActionUnlock },
		{ 197, "MOVEGLOBAL", NULL },
		{ 198, "STARTDIALOGNOSET", RunActionDialog },
		{ 199, "TEXTSCREEN", NULL },
		{ 200, "RANDOMWALKCONTINUOUS", NULL },
		{ 201, "DETECTSECRETDOOR", NULL },
		{ 202, "FADETOCOLOR", RunActionFadeToColor },
		{ 203, "FADEFROMCOLOR", RunActionFadeFromColor },
		{ 204, "TAKEPARTYITEMNUM", NULL },
		{ 207, "MOVETOPOINTNOINTERRUPT", RunActionWalkTo },
		{ 208, "MOVETOOBJECTNOINTERRUPT", NULL },
		{ 209, "SPAWNPTACTIVATE", NULL },
		{ 210, "SPAWNPTDEACTIVATE", NULL },
		{ 211, "SPAWNPTSPAWN", NULL },
		{ 212, "GLOBALSHOUT", NULL },
		{ 213, "STATICSTART", NULL },
		{ 214, "STATICSTOP", NULL },
		{ 215, "FOLLOWOBJECTFORMATION", NULL },
		{ 216, "ADDFAMILIAR", NULL },
		{ 217, "REMOVEFAMILIAR", NULL },
		{ 218, "PAUSEGAME", NULL },
		{ 219, "CHANGEANIMATIONNOEFFECT", NULL },
		{ 220, "TAKEITEMLISTPARTY", NULL },
		{ 221, "SETMORALEAI", NULL },
		{ 222, "INCMORALEAI", NULL },
		{ 223, "DESTROYALLEQUIPMENT", NULL },
		{ 224, "GIVEPARTYALLEQUIPMENT", NULL },
		{ 225, "MOVEBETWEENAREASEFFECT", RunActionMoveBetweenAreasEffect },
		{ 226, "TAKEITEMLISTPARTYNUM", NULL },
		{ 227, "CREATECREATUREOBJECTEFFECT", NULL },
		{ 228, "CREATECREATUREIMPASSABLE", RunActionCreateCreatureImpassable },
		{ 229, "FACEOBJECT", RunActionFaceObject },
		{ 230, "RESTPARTY", NULL },
		{ 231, "CREATECREATUREDOOR", NULL },
		{ 232, "CREATECREATUREOBJECTDOOR", NULL },
		{ 233, "CREATECREATUREOBJECTOFFSCREEN", NULL },
		{ 234, "MOVEGLOBALOBJECTOFFSCREEN", NULL },
		{ 235, "SETQUESTDONE", NULL },
		{ 236, "STOREPARTYLOCATIONS", NULL },
		{ 237, "RESTOREPARTYLOCATIONS", NULL },
		{ 238, "CREATECREATUREOFFSCREEN", NULL },
		{ 239, "MOVETOCENTEROFSCREEN", NULL },
		{ 240, "REALLYFORCESPELLDEAD", NULL },
		{ 241, "CALM", NULL },
		{ 242, "ALLY", NULL },
		{ 243, "RESTNOSPELLS", NULL },
		{ 244, "SAVELOCATION", NULL },
		{ 245, "SAVEOBJECTLOCATION", NULL },
		{ 246, "CREATECREATUREATLOCATION", NULL },
		{ 247, "SETTOKEN", NULL },
		{ 248, "SETTOKENOBJECT", NULL },
		{ 249, "SETGABBER", NULL },
		{ 250, "CREATECREATUREOBJECTCOPYEFFECT", NULL },
		{ 251, "HIDEAREAONMAP", NULL },
		{ 252, "CREATECREATUREOBJECTOFFSET", NULL },
		{ 253, "CONTAINERENABLE", NULL },
		{ 254, "SCREENSHAKE", RunActionScreenShake },
		{ 255, "ADDGLOBALS", NULL },
		{ 256, "CREATEITEMGLOBAL", NULL },
		{ 257, "PICKUPITEM", NULL },
		{ 258, "FILLSLOT", NULL },
		{ 259, "ADDXPOBJECT", NULL },
		{ 260, "DESTROYGOLD", NULL },
		{ 261, "SETHOMELOCATION", NULL },
		{ 262, "DISPLAYSTRINGNONAME", NULL },
		{ 263, "ERASEJOURNALENTRY", NULL },
		{ 264, "COPYGROUNDPILESTO", NULL },
		{ 265, "DIALOGFORCEINTERRUPT", NULL },
		{ 266, "STARTDIALOGUEINTERRUPT", NULL },
		{ 267, "STARTDIALOGNOSETINTERRUPT", NULL },
		{ 268, "REALSETGLOBALTIMER", NULL },
		{ 269, "DISPLAYSTRINGHEAD", RunActionDisplayStringHead },
		{ 270, "POLYMORPHCOPY", NULL },
		{ 271, "VERBALCONSTANTHEAD", NULL },
		{ 272, "CREATEVISUALEFFECT", RunActionCreateVisualEffect },
		{ 273, "CREATEVISUALEFFECTOBJECT", RunActionCreateVisualEffectObject },
		{ 274, "ADDKIT", NULL },
		{ 275, "STARTCOMBATCOUNTER", NULL },
		{ 276, "ESCAPEAREANOSEE", NULL },
		{ 277, "ESCAPEAREAOBJECTMOVE", NULL },
		{ 278, "TAKEITEMREPLACE", NULL },
		{ 279, "ADDSPECIALABILITY", NULL },
		{ 280, "DESTROYALLDESTRUCTABLEEQUIPMENT", NULL },
		{ 281, "REMOVEPALADINHOOD", NULL },
		{ 282, "REMOVERANGERHOOD", NULL },
		{ 283, "REGAINPALADINHOOD", NULL },
		{ 284, "REGAINRANGERHOOD", NULL },
		{ 285, "POLYMORPHCOPYBASE", NULL },
		{ 286, "HIDEGUI", RunActionHideGUI },
		{ 287, "UNHIDEGUI", RunActionUnhideGUI },
		{ 288, "SETNAME", NULL },
		{ 289, "ADDSUPERKIT", NULL },
		{ 290, "PLAYDEADINTERRUPTIBLE", NULL },
		{ 291, "MOVEGLOBALOBJECT", NULL },
		{ 292, "DISPLAYSTRINGHEADOWNER", NULL },
		{ 293, "STARTDIALOGOVERRIDE", NULL },
		{ 294, "STARTDIALOGOVERRIDEINTERRUPT", NULL },
		{ 295, "CREATECREATURECOPYPOINT", NULL },
		{ 296, "BATTLESONG", NULL },
		{ 297, "MOVETOSAVEDLOCATIONN", NULL },
		{ 298, "APPLYDAMAGE", NULL },
		{ 299, "BANTERBLOCKTIME", NULL },
		{ 300, "BANTERBLOCKFLAG", NULL },
		{ 301, "AMBIENTACTIVATE", NULL },
		{ 302, "ATTACHTRANSITIONTODOOR", NULL },
		{ 303, "DEATHMATCHPOSITIONGLOBAL", NULL },
		{ 304, "DEATHMATCHPOSITIONAREA", NULL },
		{ 305, "DEATHMATCHPOSITIONLOCAL", NULL },
		{ 306, "APPLYDAMAGEPERCENT", NULL },
		{ 307, "SG", NULL },
		{ 308, "ADDMAPNOTE", NULL },
		{ 309, "DEMOEND", NULL },
		{ 310, "MOVEGLOBALSTO", NULL },
		{ 311, "DISPLAYSTRINGWAIT", RunActionDisplayStringHead },
		{ 312, "STATEOVERRIDETIME", NULL },
		{ 313, "STATEOVERRIDEFLAG", NULL },
		{ 314, "SETRESTENCOUNTERPROBABILITYDAY", NULL },
		{ 315, "SETRESTENCOUNTERPROBABILITYNIGHT", NULL },
		{ 316, "SOUNDACTIVATE", NULL },
		{ 317, "PLAYSONG", NULL },
		{ 318, "FORCESPELLRANGE", NULL },
		{ 319, "FORCESPELLPOINTRANGE", NULL },
		{ 320, "SETPLAYERSOUND", NULL },
		{ 321, "SETAREARESTFLAG", NULL },
		{ 322, "FAKEEFFECTEXPIRYCHECK", NULL },
		{ 323, "CREATECREATUREIMPASSABLEALLOWOVERLAP", NULL },
		{ 324, "SETBEENINPARTYFLAGS", NULL },
};


std::string
GetActionName(int32 id)
{
	for (auto action: kActionsTable) {
		if (action.id == id)
			return std::string(action.name);
	}
	return "";
}


int32
GetActionID(std::string name)
{
	for (auto action: kActionsTable) {
		if (::strcasecmp(action.name, name.c_str()) == 0) {
			std::cout << "in: " << name << ", found: " << action.name << ", id: " << action.id << std::endl;
			return action.id;
		}
	}
	return -1;
}


const ActionDescriptor*
GetActionDescriptor(int32 id)
{
	static const std::unordered_map<int32, const ActionDescriptor*> sById = [] {
		std::unordered_map<int32, const ActionDescriptor*> map;
		for (const auto& descriptor : kActionsTable)
			map.emplace(descriptor.id, &descriptor);
		return map;
	}();

	auto found = sById.find(id);
	return found != sById.end() ? found->second : NULL;
}


bool
IsInstantAction(int32 id)
{
	// Same caching rationale as GetActionDescriptor(): whether an action id
	// is "instant" never changes at runtime, so avoid loading/scanning the
	// IDS resource on every single call.
	static std::unordered_map<int32, bool> sInstantCache;

	auto cached = sInstantCache.find(id);
	if (cached != sInstantCache.end())
		return cached->second;

	bool isInstant = false;
	IDSResource* instants = gResManager->GetIDS("INSTANT");
	if (instants != NULL) {
		isInstant = instants->StringForID(id) != "";
		gResManager->ReleaseResource(instants);
	}

	sInstantCache[id] = isInstant;
	return isInstant;
}

