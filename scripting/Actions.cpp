#include "Actions.h"

#include "2DAResource.h"

#include "Actor.h"
#include "Animation.h"
#include "AreaRoom.h"
#include "Container.h"
#include "Core.h"
#include "CreResource.h"
#include "Door.h"
#include "Effect.h"
#include "Game.h"
#include "GameTimer.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "IDSResource.h"
#include "ITMResource.h"
#include "Object.h"
#include "Party.h"
#include "Region.h"
#include "ResManager.h"
#include "RoomBase.h"
#include "Script.h"
#include "SpellEffect.h"
#include "SPLResource.h"
#include "STOResource.h"
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


// A point representing where 'object' is - Actor::Position() for actors,
// else the top-left of its Frame() (matching Object's generic surface,
// e.g. a Door/Container - there's no single "position" for those, but
// their frame's corner is a reasonable stand-in for "create a creature
// next to this").
static IE::point
_ObjectPosition(Object* object)
{
	Actor* actor = dynamic_cast<Actor*>(object);
	if (actor != NULL)
		return actor->Position();
	return object->Frame().LeftTop();
}


// Shared by SaveLocation/SaveObjectLocation/MoveToSavedLocation/
// GivePartyGoldGlobal - these read/write a named variable in either
// LOCALS (per-object, via Object::SetVariable()/GetVariable(), same as
// SETGLOBAL's own LOCALS branch) or GLOBAL (Core::Get()->Vars(), same
// as SG()/ADDGLOBALS() above) scope, chosen by a plain "LOCALS"/"GLOBAL"
// string parameter - unlike SETGLOBAL's string1, there's no 6-character
// scope-tag prefix involved here, so a direct case-insensitive compare
// is enough.
static int32
_GetScopedVariable(Object* sender, const char* name, const char* scope)
{
	return strcasecmp(scope, "LOCALS") == 0
		? sender->GetVariable(name) : Core::Get()->Vars().Get(name);
}


static void
_SetScopedVariable(Object* sender, const char* name, const char* scope, int32 value)
{
	if (strcasecmp(scope, "LOCALS") == 0)
		sender->SetVariable(name, value);
	else
		Core::Get()->Vars().Set(name, value);
}


// Point<->int32 packing shared by SaveLocation/SaveObjectLocation/
// MoveToSavedLocation: IESDP doesn't spell out an exact bit layout for
// how these actions pack a point into one variable, and nothing outside
// this engine ever reads them back, so any internally-consistent scheme
// works - this one just needs to round-trip through our own actions. y
// fits in 16 bits for any BG2-sized area, keeping x*65536+y well inside
// int32 range for realistic coordinates.
static int32
_PackLocation(const IE::point& point)
{
	return (int32)point.x * 65536 + point.y;
}


static IE::point
_UnpackLocation(int32 value)
{
	IE::point point;
	point.x = (int16)(value / 65536);
	point.y = (int16)(value % 65536);
	return point;
}


static void
_SetSavedLocation(Object* sender, const char* name, const char* scope, const IE::point& point)
{
	_SetScopedVariable(sender, name, scope, _PackLocation(point));
}


static IE::point
_GetSavedLocation(Object* sender, const char* name, const char* scope)
{
	return _UnpackLocation(_GetScopedVariable(sender, name, scope));
}


// ---- Native action implementations (proof of concept: one stateless, one
// with simple state, one with more involved state + resource access) ----

// SETGLOBAL(S:NAME*,S:AREA*,I:VALUE*) - stateless, completes immediately.
static void
RunActionSetGlobal(Object* sender, action_params* params, action_state& state)
{
	Variables::SetScoped(Script::GetSenderObject(sender, params), params->string1,
			params->integer1);
	state.completed = true;
}


// PLAYSOUND(S:Sound*) - stateless.
static void
RunActionPlaySound(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->PlaySound(params->string1);
	state.completed = true;
}


// VERBALCONSTANT(O:Object*,I:Constant*Sndslot) - stateless. Plays the
// target's soundset line for the given SNDSLOT.IDS slot: the slot indexes
// the target CRE's 100 character strrefs (cre_v1.htm offset 0x00a4), and
// the TLK entry for that strref carries the sound resref to play.
static void
RunActionVerbalConstant(Object* sender, action_params* params, action_state& state)
{
	state.completed = true;

	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target == NULL || target->CRE() == NULL)
		return;

	uint32 strRef = target->CRE()->SoundSetStringRef((uint32)params->integer1);
	if (strRef == 0xffffffff)
		return;

	TLKEntry* entry = IDTable::GetTLKEntry(strRef);
	if (entry == NULL)
		return;

	if (entry->sound_ref.CString()[0] != '\0')
		Core::Get()->PlaySound(entry->sound_ref);
	delete entry;
}


// STARTSTORE(S:Store*,O:Target*) - stateless. No store GUI exists yet
// (Fase 9 plan: parsing + this action only, GUI deferred) - resolves and
// logs the store's real data instead of silently no-op'ing like the
// previous NULL entry did, so the STOResource parsing path is exercised
// end-to-end from a real script trigger.
static void
RunActionStartStore(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	STOResource* store = gResManager->GetSTO(params->string1);
	if (store != NULL) {
		std::cout << "StartStore: " << params->string1 << " opened for "
			<< (target != NULL ? target->Name() : "(no target)")
			<< " - type=" << store->StoreType() << ", " << store->ItemsForSale().size()
			<< " item(s) for sale" << std::endl;
		gResManager->ReleaseResource(store);
	} else {
		std::cerr << "StartStore: store resource " << params->string1
			<< " not found" << std::endl;
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


// Posts the SpellCast/SpellCastPriest/SpellCastInnate (on the caster) and
// SpellCastOnMe (on the target) triggers once a spell's effects have been
// applied. Spell-id matching isn't implemented - any cast of the right
// category (priest/innate) matches, the same simplification already
// documented for the Heard() trigger (Script.cpp) - only whether a cast
// happened is tracked, not which spell.
static void
_PostSpellCastTriggers(Actor* caster, Object* target, const std::string& spellResourceName)
{
	trigger_entry cast("SpellCast");
	cast.round = Core::Get()->ScriptRound();
	caster->AddTrigger(cast);

	if (spellResourceName.compare(0, 4, "SPPR") == 0) {
		trigger_entry castPriest("SpellCastPriest");
		castPriest.round = Core::Get()->ScriptRound();
		caster->AddTrigger(castPriest);
	} else if (spellResourceName.compare(0, 4, "SPIN") == 0) {
		trigger_entry castInnate("SpellCastInnate");
		castInnate.round = Core::Get()->ScriptRound();
		caster->AddTrigger(castInnate);
	}

	trigger_entry castOnMe("SpellCastOnMe", caster);
	castOnMe.round = Core::Get()->ScriptRound();
	target->AddTrigger(castOnMe);
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
		std::string spellResourceName;
		try {
			spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		} catch (std::exception& e) {
			std::cerr << "ForceSpell: invalid spell id " << params->integer1
					<< ": " << e.what() << std::endl;
			gResManager->ReleaseResource(spellIDS);
			state.completed = true;
			return;
		}
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		if (spellResource == NULL) {
			std::cerr << "ForceSpell: spell resource \"" << spellResourceName
					<< "\" (id " << params->integer1 << ") not found" << std::endl;
			state.completed = true;
			return;
		}
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
						effect.resource.CString(), effect.savingThrowType,
						effect.savingThrowBonus));
				}
				gResManager->ReleaseResource(spellResource);
			}
			_PostSpellCastTriggers(actor, target, spellResourceName);
		}
		state.completed = true;
		std::cout << "duration:" << std::dec << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// APPLYSPELL(O:Target*,I:Spell*Spell) - same target/spell resolution and
// effect application as RunActionForceSpell above, but per IESDP "applied
// instantly; no casting animation is played" - unlike ForceSpell, there's
// no casting-time countdown here, it resolves in a single tick. This is
// what scripted self-buff blocks use right before engaging (e.g. a boss
// spellcaster's "ApplySpell(Myself,...) x N" opening sequence).
static void
RunActionApplySpell(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor == NULL) {
		std::cerr << "ApplySpell: NO sender Actor" << std::endl;
		state.completed = true;
		return;
	}

	IDSResource* spellIDS = gResManager->GetIDS("SPELL");
	std::string spellName = spellIDS->StringForID(params->integer1).c_str();
	std::string spellResourceName;
	try {
		spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
	} catch (std::exception& e) {
		std::cerr << "ApplySpell: invalid spell id " << params->integer1
				<< ": " << e.what() << std::endl;
		gResManager->ReleaseResource(spellIDS);
		state.completed = true;
		return;
	}
	gResManager->ReleaseResource(spellIDS);
	std::cout << "spell: " << spellName << std::endl;

	SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
	if (spellResource == NULL) {
		std::cerr << "ApplySpell: spell resource \"" << spellResourceName
				<< "\" (id " << params->integer1 << ") not found" << std::endl;
		state.completed = true;
		return;
	}

	Object* target = Script::GetTargetObject(sender, params);
	if (target == NULL)
		target = sender;
	std::cout << "target: " << target->Name() << std::endl;

	for (const spl_effect& effect : spellResource->Effects()) {
		target->AddSpellEffect(new SpellEffect(effect.opcode, sender,
			effect.parameter1, effect.parameter2, effect.duration,
			effect.resource.CString(), effect.savingThrowType,
			effect.savingThrowBonus));
	}
	gResManager->ReleaseResource(spellResource);
	_PostSpellCastTriggers(actor, target, spellResourceName);

	state.completed = true;
}


// SPELL(O:Target*,I:Spell*Spell) - same shape as RunActionForceSpell (cast
// time countdown, effect application), but per IESDP "the spell must
// currently be memorised by the caster" - unlike ForceSpell/
// ForceSpellPoint, which force-cast unconditionally (innate/special
// abilities, cutscene scripting), this is what real spellcasting scripts
// use, and it actually spends a spellbook slot (CREResource::
// ConsumeMemorizedSpell(), restored by REST/RESTPARTY below).
static void
RunActionSpell(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor == NULL) {
		std::cerr << "Spell: NO sender Actor" << std::endl;
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		IDSResource* spellIDS = gResManager->GetIDS("SPELL");
		std::string spellName = spellIDS->StringForID(params->integer1).c_str();
		std::string spellResourceName;
		try {
			spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		} catch (std::exception& e) {
			std::cerr << "Spell: invalid spell id " << params->integer1
					<< ": " << e.what() << std::endl;
			gResManager->ReleaseResource(spellIDS);
			state.completed = true;
			return;
		}
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		if (!actor->CRE()->ConsumeMemorizedSpell(spellResourceName.c_str())) {
			std::cerr << actor->Name() << ": Spell: \"" << spellResourceName
					<< "\" not currently memorized" << std::endl;
			state.completed = true;
			return;
		}

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		if (spellResource == NULL) {
			std::cerr << "Spell: spell resource \"" << spellResourceName
					<< "\" (id " << params->integer1 << ") not found" << std::endl;
			state.completed = true;
			return;
		}
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
			std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
			SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
			if (spellResource != NULL) {
				for (const spl_effect& effect : spellResource->Effects()) {
					target->AddSpellEffect(new SpellEffect(effect.opcode, sender,
						effect.parameter1, effect.parameter2, effect.duration,
						effect.resource.CString(), effect.savingThrowType,
						effect.savingThrowBonus));
				}
				gResManager->ReleaseResource(spellResource);
			}
			_PostSpellCastTriggers(actor, target, spellResourceName);
		}
		state.completed = true;
		std::cout << "duration:" << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// SAVEGAME(I:Slot*) - stateless. No save-folder/character-name structure
// (see IESDP's "save/<slot> - <name>/baldur.gam") is modeled - this
// engine just writes one file per slot in the working directory.
static void
RunActionSaveGame(Object* sender, action_params* params, action_state& state)
{
	std::string path = "savegame_slot" + std::to_string(params->integer1) + ".gam";
	if (!Game::Get()->Save(path.c_str()))
		std::cerr << "SaveGame: failed to write \"" << path << "\"" << std::endl;
	state.completed = true;
}


// REST()/RESTPARTY() - stateless. Both now check the current area's
// AreaRoom::CanRest() (see SETAREARESTFLAG below) and silently do
// nothing if resting isn't allowed there - matches Fase 10's added
// SETAREARESTFLAG, though this engine doesn't yet parse the ARE
// header's own "Rest disabled" bit as the flag's initial value (defaults
// true everywhere until a script says otherwise). This engine has no
// rest-movie/time-advancement to wait for (see IESDP: "does not play
// the rest movie or advance game time"), so both apply their
// spellbook-restoring effect immediately; the other real-Rest effects
// (healing over time, ability
// restoration) aren't implemented here and are out of scope for Phase 4
// (spellcasting), like Phase 3's other deferred items.
static void
RunActionRest(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL && (actor->Area() == NULL || actor->Area()->CanRest()))
		actor->CRE()->RestoreMemorizedSpells();
	state.completed = true;
}


static void
RunActionRestParty(Object* sender, action_params* params, action_state& state)
{
	AreaRoom* area = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (area == NULL || area->CanRest()) {
		::Party* party = Game::Get()->Party();
		for (uint16 i = 0; i < party->CountActors(); i++)
			party->ActorAt(i)->CRE()->RestoreMemorizedSpells();
	}
	state.completed = true;
}


// Shared by ADDEXPERIENCEPARTY/ADDEXPERIENCEPARTYGLOBAL - per IESDP,
// "distributed among all current living party members".
static void
_AddExperienceToParty(int32 amount)
{
	if (amount <= 0)
		return;

	::Party* party = Game::Get()->Party();
	std::vector<Actor*> living;
	for (uint16 i = 0; i < party->CountActors(); i++) {
		Actor* actor = party->ActorAt(i);
		if (!actor->IsState(STATE_DEAD))
			living.push_back(actor);
	}
	if (living.empty())
		return;

	const uint32 share = (uint32)amount / living.size();
	for (Actor* actor : living)
		actor->GainExperience(share);
}


// ADDEXPERIENCEPARTY(I:XP*) - stateless.
static void
RunActionAddExperienceParty(Object* sender, action_params* params, action_state& state)
{
	_AddExperienceToParty(params->integer1);
	state.completed = true;
}


// ADDEXPERIENCEPARTYGLOBAL(S:Name*,S:Area*) - the XP amount is read from a
// global variable instead of a literal; like RunActionSetGlobal(), area
// scoping isn't modeled by this engine yet, so `Area` is ignored.
static void
RunActionAddExperiencePartyGlobal(Object* sender, action_params* params, action_state& state)
{
	_AddExperienceToParty(Core::Get()->Vars().Get(params->string1));
	state.completed = true;
}


// ADDXPOBJECT(O:Object*,I:XP*) - stateless.
static void
RunActionAddXPObject(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL && params->integer1 > 0)
		target->GainExperience(params->integer1);
	state.completed = true;
}


// Actor::Actor() throws if the given resref doesn't resolve to a real
// CRE resource (a typo'd/missing resref in a script, or a bad test) -
// every CreateCreature* action below goes through this instead of
// calling `new Actor()` directly, so a bad resref just fails that one
// action (logged) instead of taking the whole process down with it (the
// exception would otherwise propagate uncaught out of
// Object::_ExecuteAction()/Game::Loop() - found the hard way while
// testing this batch).
static Actor*
_CreateActor(const char* creResRef, const IE::point& point, int face)
{
	try {
		return new Actor(creResRef, point, face);
	} catch (std::exception& e) {
		std::cerr << "CreateCreature: failed to create \"" << creResRef
			<< "\": " << e.what() << std::endl;
		return NULL;
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
	Actor* actor = _CreateActor(params->string1, point, params->integer1);
	if (actor != NULL)
		((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	state.completed = true;
}


// CREATECREATUREIMPASSABLE(S:NewObject*,P:Location*,I:Face*) - stateless.
static void
RunActionCreateCreatureImpassable(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = _CreateActor(params->string1, params->where, params->integer1);
	if (actor != NULL) {
		std::cout << "Created actor (IMPASSABLE) " << params->string1 << " on ";
		std::cout << params->where.x << ", " << params->where.y << std::endl;
		((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	}
	state.completed = true;
}


// Shared by CreateCreatureObjectEffect/CreateCreatureObjectDoor/
// CreateCreatureObjectOffScreen/CreateCreatureObjectCopyEffect
// (227/232/233/250) - IESDP: all four create a creature next to a
// target object, facing controlled by Usage1/integer1. None of their
// extra nuances are modeled: the dimension-door delay+graphic (232/233),
// off-screen placement (233), the S:Effect* resource (227/250), or
// copying the active creature's animation (250) - same "create it, skip
// the cosmetic/timing details" simplification CREATECREATUREIMPASSABLE
// above already uses for its own overlap-ignoring placement.
static void
RunActionCreateCreatureNearObject(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL) {
		Actor* actor = _CreateActor(params->string1, _ObjectPosition(target), params->integer1);
		if (actor != NULL)
			((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	}
	state.completed = true;
}


// CreateCreatureObjectOffset(S:ResRef*,O:Object*,P:Offset*) - stateless.
static void
RunActionCreateCreatureObjectOffset(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL) {
		IE::point point = _ObjectPosition(target);
		point.x += params->where.x;
		point.y += params->where.y;
		Actor* actor = _CreateActor(params->string1, point, 0);
		if (actor != NULL)
			((AreaRoom*)Core::Get()->CurrentRoom())->AddObject(actor);
	}
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


// Percentile thief-skill check against a difficulty rating (both on the
// 0-100 scale CRE skill bytes and door/container difficulty fields use):
// rolls d100, succeeds if the roll is <= (skill - difficulty), clamped to
// a 5% minimum chance so an outmatched attempt is never mathematically
// impossible. The original engine's exact formula isn't documented; this
// is a reasonable, deliberately simple approximation (same spirit as this
// codebase's other undocumented-formula choices, e.g. HP:Damage's
// discarded modes).
static bool
_RollSkillCheck(uint8 skill, uint32 difficulty)
{
	int32 chance = std::max((int32)skill - (int32)difficulty, 5);
	return Core::RollDice(1, 100, 0) <= chance;
}


// Moves one item (by resref) from 'from' to 'to'. Actor::RemoveItem() is
// destructive (it doesn't know whether the item will actually end up
// anywhere), so a naive RemoveItem()-then-AddItem() sequence silently
// destroys the item whenever 'to' has no room (no free Items-table entry
// or general slot - see Actor::AddItem()'s header comment) - discovered
// via GIVEITEM/GETITEM already having this bug while testing the new
// TAKEPARTYITEM* actions below, which would otherwise have copied it.
// Puts the item back on 'from' if the add fails, so a full-inventory
// recipient just means the transfer doesn't happen, not that the item
// vanishes.
static bool
_TransferItem(Actor* from, Actor* to, const res_ref& itemName)
{
	if (!from->RemoveItem(itemName))
		return false;
	if (!to->AddItem(itemName)) {
		from->AddItem(itemName);
		return false;
	}
	return true;
}


// UNLOCK(O:OBJECT*) - stateless. Always succeeds if the sender is
// carrying the door's key item (matching the real game's "keys just
// work" convention); otherwise resolves an Open Locks skill check against
// the door's lock difficulty.
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

	if (door->IsLocked()) {
		Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
		bool hasKey = actor != NULL && door->KeyItem().CString()[0] != '\0'
			&& actor->CRE()->FindItemSlot(door->KeyItem()) >= 0;
		if (hasKey || (actor != NULL
				&& _RollSkillCheck(actor->CRE()->OpenLocksSkill(), door->LockDifficulty())))
			door->Unlock(actor);
	}
	state.completed = true;
}


// PICKLOCK(O:Object*) - stateless. Resolves an Open Locks skill check
// against the door's lock difficulty - unlike UNLOCK, no key-item
// shortcut, since this is explicitly a "pick the lock" attempt. Posts
// PickLockFailed on the door when the roll fails.
static void
RunActionPickLock(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	Door* door = dynamic_cast<Door*>(target);
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (door != NULL && actor != NULL && door->IsLocked()) {
		if (_RollSkillCheck(actor->CRE()->OpenLocksSkill(), door->LockDifficulty()))
			door->Unlock(actor);
		else
			door->AddTrigger(trigger_entry("PickLockFailed", actor));
	}
	state.completed = true;
}


// FINDTRAPS() - stateless. Real IESDP semantics are a persistent "Detect
// Traps" modal state that keeps checking every tick (see
// docs/iesdp-gh-pages/scripting/actions/bg2actions.htm#13); this engine's
// action model only supports single-shot actions (same simplification
// already used by REST()), so this resolves a one-time Find Traps skill
// check against every trapped, not-yet-detected door in the current area
// instead. Containers aren't scanned - no lock/trap API on Container yet.
static void
RunActionFindTraps(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	AreaRoom* room = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (actor != NULL && room != NULL) {
		for (Door* door : room->Doors()) {
			if (door->IsTrapped() && !door->IsTrapDetected()
					&& _RollSkillCheck(actor->CRE()->FindTrapsSkill(), door->TrapDetectionDifficulty()))
				door->SetTrapDetected(true);
		}
	}
	state.completed = true;
}


// REMOVETRAPS(O:Trap*) - stateless. Resolves a Find/Disarm Traps skill
// check against the target's trap-removal difficulty; on success clears
// its trapped flag. Doesn't require the trap to have been detected first
// via FINDTRAPS() - a deliberate simplification (see its comment).
static void
RunActionRemoveTraps(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	Door* door = dynamic_cast<Door*>(target);
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (door != NULL && actor != NULL && door->IsTrapped()
			&& _RollSkillCheck(actor->CRE()->FindTrapsSkill(), door->TrapRemovalDifficulty()))
		door->DisarmTrap(actor);
	state.completed = true;
}


// DETECTSECRETDOOR(O:Object*) - stateless. Resolves a Find Traps skill
// check (reused as this engine's general "search" stat, matching 2E's
// single Find/Remove Traps skill governing perception checks) against the
// door's secret-door detection difficulty; on success sets its Detected
// flag. Note: nothing in this engine's rendering/hit-testing layer yet
// hides undetected secret doors, so this currently has no visible effect
// until that's added separately.
static void
RunActionDetectSecretDoor(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	Door* door = dynamic_cast<Door*>(target);
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (door != NULL && actor != NULL && door->IsSecret() && !door->IsDetected()
			&& _RollSkillCheck(actor->CRE()->FindTrapsSkill(), door->DetectionDifficulty()))
		door->SetDetected(true);
	state.completed = true;
}


// CLEARACTIONS(O:Object*) - stateless.
static void
RunActionClearActions(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL)
		target->ClearActionList();
	state.completed = true;
}


// MORALESET(O:Target*,I:Morale*)/MORALEINC/MORALEDEC - stateless. Morale
// isn't clamped here (matching this engine's general "store what the data
// says" approach elsewhere) - nothing yet reads it for AI decisions (see
// the Fase 10 plan notes on PANIC/TURN/GROUPATTACK still being NULL).
static void
RunActionMoraleSet(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetMorale((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionMoraleInc(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL) {
		int32 morale = (int32)target->CRE()->Morale() + params->integer1;
		target->CRE()->SetMorale((uint8)std::max(morale, 0));
	}
	state.completed = true;
}


static void
RunActionMoraleDec(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL) {
		int32 morale = (int32)target->CRE()->Morale() - params->integer1;
		target->CRE()->SetMorale((uint8)std::max(morale, 0));
	}
	state.completed = true;
}


// REPUTATIONSET(I:Reputation*)/REPUTATIONINC - stateless. This engine
// reads reputation per-actor (CREResource::Reputation(), see the REPUTATION
// trigger cases in Script.cpp) rather than as one true party-wide stat, so
// these apply to every current party member to keep that reading
// consistent regardless of which member a script happens to check.
static void
RunActionReputationSet(Object* sender, action_params* params, action_state& state)
{
	::Party* party = Game::Get()->Party();
	for (uint16 i = 0; i < party->CountActors(); i++)
		party->ActorAt(i)->CRE()->SetReputation((sint8)params->integer1);
	state.completed = true;
}


static void
RunActionReputationInc(Object* sender, action_params* params, action_state& state)
{
	::Party* party = Game::Get()->Party();
	for (uint16 i = 0; i < party->CountActors(); i++) {
		CREResource* cre = party->ActorAt(i)->CRE();
		cre->SetReputation((sint8)(cre->Reputation() + params->integer1));
	}
	state.completed = true;
}


// DESTROYGOLD(I:Gold*) - stateless. Per IESDP, this affects the active
// creature's own gold stat (not Party Gold) - DestroyGold(0) empties it.
static void
RunActionDestroyGold(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		CREResource* cre = actor->CRE();
		if (params->integer1 <= 0)
			cre->SetGold(0);
		else
			cre->SetGold((uint32)std::max((int64)cre->Gold() - params->integer1, (int64)0));
	}
	state.completed = true;
}


// TAKEPARTYGOLD(I:Amount*)/GIVEPARTYGOLD/GIVEGOLDFORCE - stateless. "Party
// Gold" (distinct from any one creature's own CRE gold stat, see
// DESTROYGOLD above) isn't tracked as its own concept anywhere in this
// engine yet - modeled here as a "GOLD" GLOBAL variable, reusing the
// Variables mechanism SETGLOBAL already relies on, rather than inventing
// new per-party state. Simplification: GivePartyGold's "creature must
// have it in its money variable" requirement, and GiveGoldForce's
// negative-amount-removes-from-the-creature-instead special case, aren't
// modeled - all three just add/subtract the party's GOLD global directly.
static const char* const kPartyGoldVariable = "GOLD";

static void
RunActionTakePartyGold(Object* sender, action_params* params, action_state& state)
{
	Variables& vars = Core::Get()->Vars();
	int32 gold = vars.Get(kPartyGoldVariable) - params->integer1;
	vars.Set(kPartyGoldVariable, std::max(gold, 0));
	state.completed = true;
}


static void
RunActionGivePartyGold(Object* sender, action_params* params, action_state& state)
{
	Variables& vars = Core::Get()->Vars();
	vars.Set(kPartyGoldVariable, vars.Get(kPartyGoldVariable) + params->integer1);
	state.completed = true;
}


// SETNAME(I:STRREF*) - stateless.
static void
RunActionSetName(Object* sender, action_params* params, action_state& state)
{
	Object* actor = Script::GetSenderObject(sender, params);
	if (actor != NULL)
		actor->SetName(IDTable::GetDialog(params->integer1).c_str());
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
		std::string spellResourceName;
		try {
			spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
		} catch (std::exception& e) {
			std::cerr << "ForceSpellPoint: invalid spell id " << params->integer1
					<< ": " << e.what() << std::endl;
			gResManager->ReleaseResource(spellIDS);
			state.completed = true;
			return;
		}
		gResManager->ReleaseResource(spellIDS);
		std::cout << "spell: " << spellName << std::endl;

		SPLResource* spellResource = gResManager->GetSPL(spellResourceName.c_str());
		if (spellResource == NULL) {
			std::cerr << "ForceSpellPoint: spell resource \"" << spellResourceName
					<< "\" (id " << params->integer1 << ") not found" << std::endl;
			state.completed = true;
			return;
		}
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
						effect.resource.CString(), effect.savingThrowType,
						effect.savingThrowBonus));
				}
				gResManager->ReleaseResource(spellResource);
			}
			_PostSpellCastTriggers(actor, target, spellResourceName);
		}
		state.completed = true;
		std::cout << "duration:" << (Timer::Ticks() - state.startTick) << std::endl;
	}
}


// MOVEBETWEENAREASEFFECT(S:AREA*,S:EFFECT*,P:LOCATION*,I:FACE*) - resolves
// and completes in a single tick (mirrors the original, which never left
// state.initiated false for more than one call).
// The "different area" branch hands the actor to Game::TempState (keyed
// by destination area, drained by AreaRoom::_LoadActors() once that area
// loads) and then actually triggers the load via Core::LoadArea() - the
// missing half of what used to be a "BUG: IMPLEMENT MOVING TO AREAS"
// stub: the actor got stashed, but nothing ever asked to switch areas,
// so it just sat in TempState until some unrelated area load happened to
// pick it up. S:EFFECT* (a transition visual/sound) isn't modeled, same
// spirit as other cosmetic-only parameters already skipped elsewhere.
static void
RunActionMoveBetweenAreasEffect(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		Actor* actor = dynamic_cast<Actor*>(sender);
		if (actor != NULL) {
			if (::strcasecmp(params->string1, actor->Area()->Name()) != 0) {
				Game::TempState* tempState = Game::Get()->GetTempState();
				actor->Acquire();
				Game::TempState::PendingActor pending = {
					actor, params->where, (uint16)params->integer1
				};
				tempState->actors[params->string1].push_back(pending);
				actor->Area()->RemoveObject(actor);
				// Deferred - see RunActionChangeArea()'s own comment,
				// same reasoning applies here (also runs from inside
				// AreaRoom::Update()'s actor loop).
				Core::Get()->RequestAreaChange(params->string1, "", "");
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


// MoveToOffset(P:Offset*) - state: like MOVETOPOINT above, but the
// destination is computed once (state.initiated), relative to the
// active creature's position when the action starts, rather than being
// an absolute point.
static void
RunActionMoveToOffset(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		IE::point destination = actor->Position();
		destination.x += params->where.x;
		destination.y += params->where.y;
		actor->SetDestination(destination);
		state.initiated = true;
	}

	if (!actor->MoveToNextPointInPath(false))
		state.completed = true;
}


// MOVETOOBJECT(O:Target*) / MOVETOOBJECTNOINTERRUPT(O:Target*) - same run
// function for both ids (mirrors RunActionWalkTo/MOVETOPOINTNOINTERRUPT
// above). state.point tracks the destination the current path was last
// aimed at - SetDestination() only re-runs (a real, potentially
// expensive A* search - measured ~750ms/35k expanded nodes for a single
// cross-area call) when that destination has actually moved by more
// than PointSufficientlyClose()'s threshold, not on every tick
// regardless. NearestPoint() only shifts with the *caller's* own
// position when it crosses to the target's other side, so for a normal
// single-direction approach this reduces to "compute once" just like
// MOVETOPOINT above; it still re-paths promptly if the target itself
// walks away, which is what this per-tick recompute was actually for.
// Recomputing unconditionally on every tick made any long walk (e.g.
// crossing Candlekeep's courtyard to reach an NPC) take minutes of
// wall-clock time instead of seconds.
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
	if (!state.initiated || !PointSufficientlyClose(state.point, destination)) {
		if (!PointSufficientlyClose(actor->Position(), destination))
			actor->SetDestination(destination);
		state.point = destination;
		state.initiated = true;
	}

	bool canInterrupt = params->id != 208; // 208 = MOVETOOBJECTNOINTERRUPT
	actor->SetInterruptable(canInterrupt);

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
	Object* scopeObject = Script::GetSenderObject(sender, params);
	int32 value = Variables::GetScoped(scopeObject, params->string1);
	Variables::SetScoped(scopeObject, params->string1, value + params->integer1);
	state.completed = true;
}


// LEAVEAREALUA(S:Area*,S:Entrance*,P:Point*,I:Face*) - stateless.
// Deferred via Core::RequestAreaChange() rather than loading immediately -
// this action runs from inside AreaRoom::Update()'s own actor-update loop
// (Actor::Update() -> ... -> here), and LoadArea() destroys that same
// AreaRoom (via GUI::Clear()) - a real, reproduced heap-use-after-free
// once that loop tried to continue. See RequestAreaChange()'s own comment.
//
// Party members are carried over to the new area unconditionally,
// regardless of which action triggered the change (see AreaRoom::
// _LoadActors()'s own party loop) - but a non-party actor calling this
// on itself (e.g. Gorion escorting the party out of Candlekeep in BG1's
// opening) has no such safety net. Real content calls this individually
// on every actor that needs to make the trip (confirmed against
// Ch1cut01.BCS: repeated LEAVEAREALUA calls, one per traveling actor),
// so it needs the same TempState-based transfer RunActionMoveBetween
// AreasEffect() already uses for non-party actors - without it, that
// actor was simply left behind in the old area's AreaCache, gone from
// both areas' object lookups.
static void
RunActionChangeArea(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(sender);
	if (actor != NULL && !actor->InParty()
			&& ::strcasecmp(params->string1, actor->Area()->Name()) != 0) {
		Game::TempState* tempState = Game::Get()->GetTempState();
		actor->Acquire();
		Game::TempState::PendingActor pending = {
			actor, params->where, (uint16)params->integer1
		};
		tempState->actors[params->string1].push_back(pending);
		actor->Area()->RemoveObject(actor);
	}

	Core::Get()->RequestAreaChange(params->string1, "", params->string2);
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


// BASHDOOR(O:OBJECT*) - stateless. IESDP has no documented success
// formula for this ("attempt to bash the specified door") - reuses
// _RollSkillCheck() (same "reasonable, deliberately simple
// approximation" spirit already used for lockpicking/trap checks
// above) against Strength instead of a thief skill, since bashing is a
// strength check, not a dexterity one; Strength is 3-25 in this
// engine's data, so it's scaled up (*4) to sit on the same 0-100 range
// _RollSkillCheck()/LockDifficulty() expect. On success (or if the
// door wasn't locked to begin with) it unlocks and opens the door; on
// failure the door is left exactly as it was, same as a failed
// PICKLOCK.
static void
RunActionBashDoor(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	Door* door = dynamic_cast<Door*>(target);
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (door != NULL && actor != NULL) {
		std::cout << "actor " << actor->Name() << " bashes " << door->Name() << std::endl;
		if (door->IsLocked()) {
			BaseAttributes attributes;
			actor->CRE()->GetAttributes(attributes);
			if (_RollSkillCheck((uint8)(attributes.strength * 4), door->LockDifficulty())) {
				std::cout << "\tsucceeded" << std::endl;
				door->Unlock(actor);
			} else
				std::cout << "\tfailed" << std::endl;
		}
		if (!door->IsLocked() && !door->Opened())
			door->Open(actor);
	}
	state.completed = true;
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
	GUI::Get()->DisplayMessage(NULL, dialogText);
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

	// Same expensive-recompute-every-tick fix as RunActionWalkToObject()
	// above: only re-run SetDestination() (a real A* search) when the
	// target has actually moved far enough for the old path to be
	// stale, not unconditionally on every tick regardless of distance.
	IE::point point = target->NearestPoint(actorSender->Position());
	if (!state.flag || !PointSufficientlyClose(state.point, point)) {
		if (!PointSufficientlyClose(actorSender->Position(), point))
			actorSender->SetDestination(point);
		state.point = point;
		state.flag = true;
	}

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


// KILL(O:Object*) - stateless. Routes through Actor::ApplyDamage() so
// death gets the same HP/animation/DeathVariable transition as combat,
// instead of duplicating that logic here. Item-dropping (per IESDP) isn't
// implemented, matching this codebase's lack of any other loot-drop path.
static void
RunActionKill(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->ApplyDamage(target->CRE()->CurrentHitPoints());
	state.completed = true;
}


// APPLYDAMAGE(O:Object*,I:Amount*,I:Type*DMGTYPE) - stateless. Damage
// type (integer2) is ignored: resistances aren't implemented yet (see
// Actor::ApplyDamage()).
static void
RunActionApplyDamage(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->ApplyDamage(params->integer1);
	state.completed = true;
}


// CREATEITEM(S:ResRef*,I:Usage1*,I:Usage2*,I:Usage3*) - stateless. Only
// Usage1 (integer1) is used as a quantity/charge count (Actor::AddItem()
// doesn't model per-charge-type usage separately); 0 means "1".
static void
RunActionCreateItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->AddItem(params->string1, params->integer1 != 0 ? params->integer1 : 1);
	state.completed = true;
}


// DESTROYITEM(S:ResRef*) - stateless.
static void
RunActionDestroyItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->RemoveItem(params->string1);
	state.completed = true;
}


// DROPITEM(S:Object*,P:Location*) - stateless. Takes the named item out
// of the active creature's inventory and leaves it as a loose pile on
// the area floor (AreaRoom::AddGroundItem()), at the Location parameter
// or, if none was given, at the creature's own feet. Session-only piles
// (see IE::ground_pile) - not written to the area checkpoint or a save.
static void
RunActionDropItem(Object* sender, action_params* params, action_state& state)
{
	state.completed = true;
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL || actor->CRE() == NULL)
		return;

	int32 slot = actor->CRE()->FindItemSlot(params->string1);
	if (slot < 0)
		return;

	IE::item item;
	if (!actor->TakeItemFromSlot((uint32)slot, item))
		return;

	IE::point where = params->where;
	if (where.x == 0 && where.y == 0)
		where = actor->Position();
	if (actor->Area() != NULL)
		actor->Area()->AddGroundItem(item, where);
}


// GIVEITEM(S:Object*,O:Target*) - stateless. Moves the item from the
// active creature's inventory to the target's.
static void
RunActionGiveItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (actor != NULL && target != NULL)
		_TransferItem(actor, target, params->string1);
	state.completed = true;
}


// GETITEM(S:Object*,O:Target*) - stateless. The inverse of GIVEITEM: the
// active creature takes the item from the target's inventory.
static void
RunActionGetItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (actor != NULL && target != NULL)
		_TransferItem(target, actor, params->string1);
	state.completed = true;
}


// EQUIPITEM(S:Object*) - stateless.
static void
RunActionEquipItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->EquipItem(params->string1);
	state.completed = true;
}


// USEITEMSLOT(O:Target*,I:Slot*) - stateless. Applies the item's first
// ability's on-hit effects to the target (same effect-application path
// ForceSpell() uses) and consumes one charge, removing the item once its
// charges run out. Ability selection (UseItemSlotAbility's 3rd parameter)
// isn't implemented - always ability 0.
static void
RunActionUseItemSlot(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Object* target = Script::GetTargetObject(sender, params);
	if (actor == NULL || target == NULL) {
		state.completed = true;
		return;
	}

	IE::item item;
	if (actor->CRE()->GetItemAtSlot(params->integer1, item)) {
		ITMResource* itm = gResManager->GetITM(item.name);
		if (itm != NULL) {
			for (const spl_effect& effect : itm->OnHitEffects(0)) {
				target->AddSpellEffect(new SpellEffect(effect.opcode, actor,
					effect.parameter1, effect.parameter2, effect.duration,
					effect.resource.CString(), effect.savingThrowType,
					effect.savingThrowBonus));
			}
			gResManager->ReleaseResource(itm);
		}

		if (item.quantity1 > 1) {
			item.quantity1--;
			actor->CRE()->SetItemAtItemsIndex(
				(uint16)actor->CRE()->ItemsIndexAtSlot(params->integer1), item);
		} else {
			actor->RemoveItem(item.name);
		}
	}

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
	// Pass `sender` as the actor to spare: IE semantics clear everyone
	// *else*, so a cutscene beat can queue ClearAllActions() before its
	// own remaining actions and still have those run.
	if (sender != NULL && sender->Area() != NULL)
		sender->Area()->ClearAllActions(sender);
	state.completed = true;
}


// SETGLOBALTIMER(S:NAME*,S:AREA*,I:TIME*GTIMES) - stateless. Measured
// against CINGAME (GameTimer's default TIMER_GLOBAL clock, in AI ticks -
// see GameTimer.h's header comment on the two-clock model). Per IESDP's
// own timer documentation (appendices/timers.htm - "Time is tracked in
// ticks... SetGlobalTimer("TINGAME","GLOBAL",3600) - TINGAME will expire
// after half a day of in-game time"), the Time parameter is already in
// CINGAME ticks, not seconds - no AI_UPDATE_FREQ conversion needed (that
// belongs to STARTTIMER below, whose own Time parameter IESDP describes
// as seconds). Confirmed on real data: GTimes.ids' ONE_DAY is 7200 -
// exactly double IESDP's "half a day" example (3600) - and multiplying
// it by AI_UPDATE_FREQ used to land on GTimes.ids' own FIFTEEN_DAYS
// entry (7200*15=108000), turning a one-day timer into fifteen days.
static void
RunActionSetGlobalTimer(Object* sender, action_params* params, action_state& state)
{
	std::string timerName;
	// TODO: We append the timer name to the area name, check if it's okay
	timerName.append(params->string2).append(params->string1);
	GameTimer::Add(timerName.c_str(), params->integer1);
	state.completed = true;
}


// RealSetGlobalTimer(S:Name*,S:Area*,I:Time*GTimes) - stateless.
// Previously an alias of SETGLOBALTIMER above (Fase 10 batch 7); now
// that GameTimer models CREAL as a real second-based clock separate
// from CINGAME (GameTimer::TIMER_REAL, seeded from Timer::Ticks()/
// SDL_GetTicks() rather than the AI-tick-counted CINGAME), this writes
// a genuinely distinct timer - integer1 is real seconds directly, no
// AI_UPDATE_FREQ conversion (CREAL is already in seconds).
static void
RunActionRealSetGlobalTimer(Object* sender, action_params* params, action_state& state)
{
	std::string timerName;
	timerName.append(params->string2).append(params->string1);
	GameTimer::Add(timerName.c_str(), params->integer1, TIMER_REAL);
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


// DISPLAYSTRINGHEAD(O:OBJECT*,I:STRREF*) / DISPLAYSTRINGWAIT(...)
static void
RunActionDisplayStringHead(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		state.initiated = true;
		state.counter = 50; // TODO: Should be based on text length
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


// ChangeGeneral/Race/Class/Specifics/Gender/Alignment(O:Object*,I:Value*)
// - stateless. Each just writes the corresponding CRE stat byte (same
// fields the GENERAL/RACE/CLASS/SPECIFICS/GENDER/ALIGNMENT triggers -
// Actor::IsGeneral() etc. - already compare against), no validation
// against the *.IDS table (matching this engine's general "store what
// the script says" approach elsewhere, e.g. MORALESET).
static void
RunActionChangeGeneral(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetGeneral((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionChangeRace(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetRace((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionChangeClass(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetClass((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionChangeSpecifics(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetSpecific((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionChangeGender(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetGender((uint8)params->integer1);
	state.completed = true;
}


static void
RunActionChangeAlignment(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetAlignment((uint8)params->integer1);
	state.completed = true;
}


// JoinParty()/LeaveParty() - stateless. Just add/remove the active
// creature from Game::Party() - Actor::InParty() already reads this
// list live, so every InParty()-based trigger picks the change up
// immediately with no extra state to keep in sync. The creature stays
// in its AreaRoom's actor list either way (still drawn/scripted/moved
// normally) - only its "is it a party member" status changes.
// Deviations: JoinParty()'s "if the party is full, show the 'select
// party members' dialog" isn't modeled (no such GUI screen exists yet -
// see the Fase 6 plan notes); LeaveParty()'s implicit DropInventory()
// call isn't either (ground items aren't modeled - same simplification
// already declared for DROPITEM/GIVEITEM above).
static void
RunActionJoinParty(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL && !Game::Get()->Party()->HasActor(actor)) {
		Game::Get()->Party()->AddActor(actor);
		actor->ClearActionList();
	}
	state.completed = true;
}


static void
RunActionLeaveParty(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		Game::Get()->Party()->RemoveActor(actor);
	state.completed = true;
}


// Lock(O:Object*) - stateless. Inverse of Unlock(): the target door
// becomes locked again (no skill check involved either way - same target
// resolution Unlock()/PickLock() already use).
static void
RunActionLock(Object* sender, action_params* params, action_state& state)
{
	Door* door = dynamic_cast<Door*>(Script::GetTargetObject(sender, params));
	if (door != NULL)
		door->Lock();
	state.completed = true;
}


// JumpToPoint(P:Point*) - stateless. Instant teleport, no path/movement -
// distinct from MoveToPoint (Actions.cpp's RunActionWalkTo), which walks
// there over time.
static void
RunActionJumpToPoint(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		actor->SetPosition(params->where);
		actor->ClearDestination();
	}
	state.completed = true;
}


// PauseGame() - stateless. Core::TogglePause() toggles, so this only
// flips it when not already paused - PauseGame() must set the paused
// state, not toggle it (calling it twice must not unpause).
static void
RunActionPauseGame(Object* sender, action_params* params, action_state& state)
{
	if (!Core::Get()->IsPaused())
		Core::Get()->TogglePause();
	state.completed = true;
}


// IncrementChapter(S:ResRef*) - stateless. Per IESDP this also displays a
// text screen named by the resref parameter - no such GUI screen exists
// yet (see the Fase 6 plan notes), so only the chapter counter itself
// (read by scripts as Global("Chapter","GLOBAL",n), per the IESDP example)
// is modeled, reusing the Variables mechanism the Fase 10 batch 1 GOLD
// global already relies on.
static const char* const kChapterVariable = "Chapter";

static void
RunActionIncrementChapter(Object* sender, action_params* params, action_state& state)
{
	Variables& vars = Core::Get()->Vars();
	vars.Set(kChapterVariable, vars.Get(kChapterVariable) + 1);
	state.completed = true;
}


// GiveItemCreate(S:ResRef*,O:Object*,I:Usage1*,I:Usage2*,I:Usage3*) -
// stateless. Same simplification as CREATEITEM (only Usage1/integer1 is
// used as a quantity), but the item is created on the target object
// instead of the active creature.
static void
RunActionGiveItemCreate(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->AddItem(params->string1, params->integer1 != 0 ? params->integer1 : 1);
	state.completed = true;
}


// ChangeEnemyAlly(O:Object*,I:Value*EA) - stateless. CREResource::
// SetEnemyAlly() already existed (used by the Set-EnemyAlly console
// command), just wasn't wired to a script action yet.
static void
RunActionChangeEnemyAlly(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->CRE()->SetEnemyAlly((uint8)params->integer1);
	state.completed = true;
}


// SG(S:Name*,I:Num*) - stateless. Per IESDP "a shortcut for SetGlobal(),
// can only set global variables" - unlike SETGLOBAL, there's no scope
// parameter at all, so this can't reuse RunActionSetGlobal(): that
// function's GLOBAL-scope branch is fine (it already just forwards
// params->string1/integer1 to Vars().Set()), but its LOCALS-scope
// detection (Variables::GetNameAndScope(), which assumes the first 6
// characters of the string are a scope tag written by SETGLOBAL's own
// parsing) would misinterpret SG's plain variable name and corrupt it.
static void
RunActionSG(Object* sender, action_params* params, action_state& state)
{
	Core::Get()->Vars().Set(params->string1, params->integer1);
	state.completed = true;
}


// AddGlobals(S:Name*,S:Name2*) - stateless. Per IESDP "only works for
// variables in the GLOBAL scope" - same direct Vars() access as SG()
// above, no LOCALS scope-string parsing.
static void
RunActionAddGlobals(Object* sender, action_params* params, action_state& state)
{
	Variables& vars = Core::Get()->Vars();
	vars.Set(params->string1, vars.Get(params->string1) + vars.Get(params->string2));
	state.completed = true;
}


// SetNumTimesTalkedTo(I:Num*) - stateless.
static void
RunActionSetNumTimesTalkedTo(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->SetNumTimesTalkedTo((uint32)params->integer1);
	state.completed = true;
}


// DropInventory() / DestroyAllEquipment() - same run function for both:
// under this engine's simplification (no "item lying on the ground"
// object type - see GIVEITEM/DROPITEM above) they're equivalent, since
// neither actually places anything anywhere. Real DropInventory() is
// also called internally by LeaveParty() - not modeled there either, same
// declared deviation as Fase 10 batch 2's LeaveParty() note.
static void
RunActionClearInventory(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->ClearInventory();
	state.completed = true;
}


// GivePartyAllEquipment() - stateless. Gives every item the active
// creature owns to the first *other* party member found (IESDP doesn't
// specify a distribution rule beyond "give to the party" - this engine
// has no "item lying on the ground"/free-for-all-drop concept, so one
// concrete recipient is picked rather than splitting across everyone).
// A no-op if the active creature isn't in a party, or is the only member.
static void
RunActionGivePartyAllEquipment(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL && actor->InParty()) {
		::Party* party = Game::Get()->Party();
		for (uint16 i = 0; i < party->CountActors(); i++) {
			Actor* member = party->ActorAt(i);
			if (member != actor) {
				actor->GiveAllItemsTo(member);
				break;
			}
		}
	}
	state.completed = true;
}


// TakePartyItem(S:Item*) / TakePartyItemRange(S:Item*) - same run
// function for both (IESDP: TakePartyItemRange "cannot specify a range",
// making it identical to TakePartyItem - same declared deviation as
// FORCESPELLRANGE/FORCESPELLPOINTRANGE above). Checks party members in
// order and takes one item (a whole stack, per Actor::RemoveItem()) from
// the first one that has it, via _TransferItem() so a full active
// creature's inventory just means the item stays put, not that it's
// destroyed.
static void
RunActionTakePartyItem(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		::Party* party = Game::Get()->Party();
		for (uint16 i = 0; i < party->CountActors(); i++) {
			Actor* member = party->ActorAt(i);
			if (member != actor && member->CRE()->FindItemSlot(params->string1) >= 0) {
				_TransferItem(member, actor, params->string1);
				break;
			}
		}
	}
	state.completed = true;
}


// TakePartyItemAll(S:Item*) - stateless. Takes every stack of the item
// from every party member (TakePartyItem above stops at the first
// member/first stack).
static void
RunActionTakePartyItemAll(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		::Party* party = Game::Get()->Party();
		bool actorFull = false;
		for (uint16 i = 0; i < party->CountActors() && !actorFull; i++) {
			Actor* member = party->ActorAt(i);
			if (member == actor)
				continue;
			while (member->CRE()->FindItemSlot(params->string1) >= 0) {
				if (!_TransferItem(member, actor, params->string1)) {
					actorFull = true;
					break;
				}
			}
		}
	}
	state.completed = true;
}


// TakePartyItemNum(S:ResRef*,I:Num*) - stateless. Simplification: Num
// counts occupied item slots taken (each Actor::RemoveItem() call takes
// one whole stack, same unit Actor::AddItem()/RemoveItem() already use
// everywhere else in this engine), not individual item charges/quantity.
static void
RunActionTakePartyItemNum(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		::Party* party = Game::Get()->Party();
		int32 remaining = params->integer1;
		for (uint16 i = 0; i < party->CountActors() && remaining > 0; i++) {
			Actor* member = party->ActorAt(i);
			if (member == actor)
				continue;
			while (remaining > 0 && member->CRE()->FindItemSlot(params->string1) >= 0) {
				if (!_TransferItem(member, actor, params->string1)) {
					remaining = 0; // actor has no room left, stop entirely
					break;
				}
				remaining--;
			}
		}
	}
	state.completed = true;
}


// Shared by StartDialogue/StartDialogOverride and their "Interrupt"
// siblings (137/293/266/294) - IESDP describes the Interrupt variants as
// identical except they're "important enough to interrupt [an]other
// dialog[] already in action", so instead of the plain variants' no-op
// guard (Game::InitiateDialog() asserts a dialog isn't already active),
// they end the current one first via TerminateDialog(). string1 empty
// (no S:DialogFile* param at all, as for PlayerDialogue/
// StartDialogNoSetInterrupt below) means "don't override the active
// creature's existing dialog file" - same as StartDialogue's own
// no-string1-given case.
static void
_StartDialogue(Object* sender, action_params* params, bool forceInterrupt)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (actor == NULL || target == NULL)
		return;

	if (Game::Get()->InDialogMode()) {
		if (!forceInterrupt)
			return;
		Game::Get()->TerminateDialog();
	}

	if (params->string1[0] != '\0')
		actor->CRE()->SetDialogFile(params->string1);
	Game::Get()->InitiateDialog(actor, target);
}


// StartDialogue(S:DialogFile*,O:Target*) / PlayerDialogue(O:Target*) /
// StartDialogOverride(S:DialogFile*,O:Target*) - same run function for
// all three (PLAYERDIALOGUE has no S:DialogFile* param, so string1 is
// naturally empty - "don't override"; STARTDIALOGOVERRIDE's IESDP text
// is otherwise identical to STARTDIALOGUE's, its "override"/"converse as
// item" nuance isn't modeled).
static void
RunActionStartDialogue(Object* sender, action_params* params, action_state& state)
{
	_StartDialogue(sender, params, false);
	state.completed = true;
}


// StartDialogueInterrupt/StartDialogNoSetInterrupt/
// StartDialogOverrideInterrupt - same run function for all three, same
// no-S:DialogFile*-param reasoning as RunActionStartDialogue above for
// StartDialogNoSetInterrupt.
static void
RunActionStartDialogueInterrupt(Object* sender, action_params* params, action_state& state)
{
	_StartDialogue(sender, params, true);
	state.completed = true;
}


// DialogForceInterrupt(O:Object*) - stateless. Same as DIALOG(8) above,
// but forcibly ends any dialog already in progress first instead of
// relying on the caller not to queue this while one is active (unlike
// DIALOG(8), which has no such guard - Game::InitiateDialog() would
// assert).
static void
RunActionDialogForceInterrupt(Object* sender, action_params* params, action_state& state)
{
	Actor* speaker = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (speaker == NULL || target == NULL
			|| speaker->IsState(STATE_DEAD) || target->IsState(STATE_DEAD)) {
		state.completed = true;
		return;
	}

	if (Game::Get()->InDialogMode())
		Game::Get()->TerminateDialog();
	Game::Get()->InitiateDialog(speaker, target);
	state.completed = true;
}


// SetDialogue(S:DialogFile*) - stateless. Only sets the active
// creature's dialog file (CREResource::SetDialogFile(), same setter
// StartDialogue uses) - unlike StartDialogue, doesn't start a dialog.
// Per IESDP, SetDialogue("") clears it - an empty params->string1
// already does that (CREResource::SetDialogFile() just writes whatever
// it's given, no "empty means don't touch" special-casing like
// StartDialogue's string1 check above).
static void
RunActionSetDialogue(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->CRE()->SetDialogFile(params->string1);
	state.completed = true;
}


// AddKit(I:Kit*KIT) - stateless. Per IESDP this also removes abilities
// granted by any previous kit, and enforces class restrictions on the new
// one - neither is modeled (this engine has no kit-ability-granting
// system yet), so this only writes the raw stat, same "store what the
// script says" approach as the other CHANGE*/stat setters above.
static void
RunActionAddKit(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->CRE()->SetKit((uint32)params->integer1);
	state.completed = true;
}


// SaveLocation(S:Area*,S:Global*,P:Point*) - stateless. string1 is the
// scope ("LOCALS"/"GLOBAL", confusingly named "Area" by IESDP here),
// string2 the variable name - see _SetSavedLocation() above.
static void
RunActionSaveLocation(Object* sender, action_params* params, action_state& state)
{
	_SetSavedLocation(sender, params->string2, params->string1, params->where);
	state.completed = true;
}


// SaveObjectLocation(S:Area*,S:Global*,O:Object*) - stateless. Same
// scope/name parameter order as SaveLocation above.
static void
RunActionSaveObjectLocation(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL)
		_SetSavedLocation(sender, params->string2, params->string1, _ObjectPosition(target));
	state.completed = true;
}


// MoveToSavedLocation(S:GLOBAL*,S:Area*) / MoveToSavedLocationN - state:
// same walk-to-point pattern as MOVETOPOINT, but the destination comes
// from a location previously stored by SaveLocation/SaveObjectLocation.
// Note the parameter order is reversed from SaveLocation's: here
// string1 is the variable name, string2 the scope (matches IESDP's own
// example: MoveToSavedLocationn("DefaultLocation","LOCALS")).
static void
RunActionMoveToSavedLocation(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		actor->SetDestination(_GetSavedLocation(actor, params->string1, params->string2));
		state.initiated = true;
	}

	if (!actor->MoveToNextPointInPath(false))
		state.completed = true;
}


// SetHomeLocation(P:Point*) - stateless. Per IESDP "stores a home
// location into memory for a creature [...] not saved" - stored as a
// plain point on the Actor itself (Actor::SetHomeLocation()/
// HomeLocation()); nothing in this engine reads it back yet (no
// "return home" AI behavior exists), same as when this was first added.
static void
RunActionSetHomeLocation(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->SetHomeLocation(params->where);
	state.completed = true;
}


// AddJournalEntry(I:Entry*,I:Type*JourType) - stateless. Section
// (Quest/Story/User, from JourType) isn't modeled - see Game::
// AddJournalEntry()'s header comment - just an ordered list of strrefs.
static void
RunActionAddJournalEntry(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->AddJournalEntry((uint32)params->integer1);
	state.completed = true;
}


// EraseJournalEntry(I:STRREF*) / SetQuestDone(I:STRREF*) - same run
// function for both: both just remove a strref from the journal (the
// "regardless of section" / "from the quest section" distinction isn't
// meaningful here since no sections are modeled - see AddJournalEntry
// above).
static void
RunActionEraseJournalEntry(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->RemoveJournalEntry((uint32)params->integer1);
	state.completed = true;
}


// SetToken(S:Token*,I:STRREF*) - stateless. Stores the resolved string
// for the strref as the token's value (Game::SetToken()); consumed by
// DialogHandler::_FillPlaceHolders(), which now actually gets called
// (see Dialog.cpp) instead of being dead code.
static void
RunActionSetToken(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->SetToken(params->string1, IDTable::GetDialog(params->integer1));
	state.completed = true;
}


// SetTokenObject(S:Token*,O:Object) - stateless. Same token store as
// SetToken above, but the value is the target object's name.
static void
RunActionSetTokenObject(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL)
		Game::Get()->SetToken(params->string1, target->Name());
	state.completed = true;
}


// SetGabber(O:Object) - stateless. Per IESDP "updates various tokens
// based on the specified object" - this engine only has the one such
// token (GABBER, the only one seen driving real dialog text so far -
// "Io sono <GABBER>" in ANOMEN10's own recruitment dialog), so this only
// sets that.
static void
RunActionSetGabber(Object* sender, action_params* params, action_state& state)
{
	Object* target = Script::GetTargetObject(sender, params);
	if (target != NULL)
		Game::Get()->SetToken("GABBER", target->Name());
	state.completed = true;
}


// GivePartyGoldGlobal(S:Name*,S:Area*) - stateless. Gives the party a
// gold amount read from a named variable (see _GetScopedVariable()
// above), deducted from the active creature's own gold stat - reuses
// the "GOLD" party-wide GLOBAL from Fase 10 batch 1's TAKEPARTYGOLD/
// GIVEPARTYGOLD/GIVEGOLDFORCE.
static void
RunActionGivePartyGoldGlobal(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	int32 amount = _GetScopedVariable(sender, params->string1, params->string2);
	if (actor != NULL && amount > 0) {
		CREResource* cre = actor->CRE();
		cre->SetGold((uint32)std::max((int64)cre->Gold() - amount, (int64)0));
	}
	if (amount > 0) {
		Variables& vars = Core::Get()->Vars();
		vars.Set(kPartyGoldVariable, vars.Get(kPartyGoldVariable) + amount);
	}
	state.completed = true;
}


// RemoveSpell(I:Spell*Spell) - stateless. Removes one memorized instance
// of the spell from the active creature's spellbook - the same
// underlying operation RunActionSpell() uses to spend a slot when
// actually casting (CREResource::ConsumeMemorizedSpell()), just without
// casting/applying any effect.
static void
RunActionRemoveSpell(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		try {
			std::string spellResourceName = SPLResource::GetSpellResourceName(params->integer1);
			actor->CRE()->ConsumeMemorizedSpell(spellResourceName.c_str());
		} catch (std::exception& e) {
			std::cerr << "RemoveSpell: invalid spell id " << params->integer1
				<< ": " << e.what() << std::endl;
		}
	}
	state.completed = true;
}


// ContainerEnable(O:Object,I:Bool*BOOLEAN) - stateless.
static void
RunActionContainerEnable(Object* sender, action_params* params, action_state& state)
{
	Container* container = dynamic_cast<Container*>(Script::GetTargetObject(sender, params));
	if (container != NULL)
		container->SetEnabled(params->integer1 != 0);
	state.completed = true;
}


// ApplyDamagePercent(O:Object*,I:Amount*,I:Type*DMGTYPE) - stateless.
// IESDP doesn't spell out exactly what the percentage is relative to -
// treated as a percentage of the target's max HP, the natural reading
// for a percent-damage spell/trap effect. Damage type isn't modeled,
// same as APPLYDAMAGE above (no resistances layer yet - see the Fase 3
// plan notes on Stat:* opcodes being explicitly deferred).
static void
RunActionApplyDamagePercent(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL) {
		int32 damage = (int32)target->CRE()->MaxHitPoints() * params->integer1 / 100;
		target->ApplyDamage(damage);
	}
	state.completed = true;
}


// EscapeAreaDestroy() / EscapeAreaNoSee() - same run function as
// ESCAPEAREA/ESCAPEAREAMOVE above (RunActionEscapeArea): removes the
// active creature via Actor::DestroySelf() - no travel-trigger
// pathfinding or line-of-sight check modeled, same simplification
// already established for that pair.


// EscapeAreaObjectMove(S:ResRef*,O:Object*,I:X*,I:Y*,I:Face*) -
// stateless. Removes the TARGET object from the area instead of the
// active creature - same DestroySelf()-based simplification.
static void
RunActionEscapeAreaObjectMove(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL)
		target->DestroySelf();
	state.completed = true;
}


// StorePartyLocations() / RestorePartyLocations() - stateless. Kept in
// memory only (Party::StoreLocations()/RestoreLocations() - see
// Party.h's header comment); RestorePartyLocations() is a no-op if
// nothing was stored or the party's membership changed size since.
static void
RunActionStorePartyLocations(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->Party()->StoreLocations();
	state.completed = true;
}


static void
RunActionRestorePartyLocations(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->Party()->RestoreLocations();
	state.completed = true;
}


// DestroyAllDestructableEquipment() - alias of DESTROYALLEQUIPMENT
// above (RunActionClearInventory) - the "only destructible items"
// filter isn't modeled (this engine doesn't track an item's
// destructible flag), so it destroys everything, same as its
// non-selective counterpart.




// ReallyForceSpell(O:Target,I:Spell*Spell) / ReallyForceSpellDead
// (O:Target,I:Spell*Spell) - alias of FORCESPELL above
// (RunActionForceSpell). Per IESDP both cast "instantly" and
// "will not be interrupted" - RunActionForceSpell already doesn't check
// interruptability, and its casting-time countdown is the only
// difference from "instant" that isn't modeled; ReallyForceSpellDead's
// ability to target dead creatures needs nothing extra either, since
// RunActionForceSpell never checks the target's state to begin with.


// TakeItemReplace(S:Give*,S:Take*,O:Object*) - stateless. Removes the
// "Take" item if present (ignored if not - the "Give" item is created
// either way, per IESDP) and adds the "Give" item. Doesn't auto-equip
// the new item, matching IESDP's own note that TakeItemReplace() itself
// doesn't.
static void
RunActionTakeItemReplace(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL) {
		target->RemoveItem(params->string2);
		target->AddItem(params->string1);
	}
	state.completed = true;
}


// TakeItemListParty(S:ResRef*) / TakeItemListPartyNum(S:ResRef*,I:Num*)
// - stateless. The 2DA lists one item resref per row (column 0, the
// simplest reading of "items listed in the specified 2DA file" - IESDP
// doesn't spell out the column layout further). TakeItemListParty
// removes every instance of each listed item found anywhere in the
// party; TakeItemListPartyNum stops after Num instances total across
// the whole list (not per item - the one IESDP example uses Num=1 with
// a single-row 2DA, which doesn't disambiguate the two readings).
// Destructive removal only (no active-creature recipient, unlike
// TAKEPARTYITEM* in Fase 10 batch 3) - IESDP doesn't mention one either.
static void
RunActionTakeItemListParty(Object* sender, action_params* params, action_state& state)
{
	TWODAResource* table = gResManager->Get2DA(params->string1);
	if (table != NULL) {
		::Party* party = Game::Get()->Party();
		for (int32 row = 0; row < table->CountRows(); row++) {
			res_ref itemName = table->ValueAt(row, 0).c_str();
			for (uint16 i = 0; i < party->CountActors(); i++) {
				Actor* member = party->ActorAt(i);
				while (member->RemoveItem(itemName))
					;
			}
		}
		gResManager->ReleaseResource(table);
	}
	state.completed = true;
}


static void
RunActionTakeItemListPartyNum(Object* sender, action_params* params, action_state& state)
{
	TWODAResource* table = gResManager->Get2DA(params->string1);
	if (table != NULL) {
		::Party* party = Game::Get()->Party();
		int32 remaining = params->integer1;
		for (int32 row = 0; row < table->CountRows() && remaining > 0; row++) {
			res_ref itemName = table->ValueAt(row, 0).c_str();
			for (uint16 i = 0; i < party->CountActors() && remaining > 0; i++) {
				Actor* member = party->ActorAt(i);
				while (remaining > 0 && member->RemoveItem(itemName))
					remaining--;
			}
		}
		gResManager->ReleaseResource(table);
	}
	state.completed = true;
}


// Calm(O:Object) - stateless. Per IESDP "reverses the effect of the
// Panic action, and may also remove other effects" - only clears
// STATE_PANIC (the Horror/Panic state bit Fase 3's SpellEffect opcode
// 24 already sets/clears); the "other effects" this may also remove
// aren't specified and aren't modeled.
static void
RunActionCalm(Object* sender, action_params* params, action_state& state)
{
	Actor* target = dynamic_cast<Actor*>(Script::GetTargetObject(sender, params));
	if (target != NULL) {
		CREResource* cre = target->CRE();
		cre->SetPermanentStatus(cre->PermanentStatus() & ~(uint32)STATE_PANIC);
	}
	state.completed = true;
}


// Ally() - stateless. Sets the active creature's allegiance to ALLY (4,
// per EA.IDS) - same CREResource::SetEnemyAlly() CHANGEENEMYALLY
// already uses in Fase 10 batch 3.
static void
RunActionAlly(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->CRE()->SetEnemyAlly(4);
	state.completed = true;
}


// DisplayStringHeadOwner(S:Item*,I:STRREF*) - state: same countdown
// pattern as DISPLAYSTRINGHEAD above, but the target is whichever
// current party member holds the item (re-resolved every tick rather
// than cached across state - simpler, and party composition rarely
// changes mid-action anyway; same "recompute, don't cache" approach
// MOVETOOBJECT already uses for its target).
static void
RunActionDisplayStringHeadOwner(Object* sender, action_params* params, action_state& state)
{
	Actor* owner = NULL;
	::Party* party = Game::Get()->Party();
	for (uint16 i = 0; i < party->CountActors() && owner == NULL; i++) {
		Actor* member = party->ActorAt(i);
		if (member->CRE()->FindItemSlot(params->string1) >= 0)
			owner = member;
	}
	if (owner == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		state.initiated = true;
		state.counter = 50; // TODO: Should be based on text length - same as DISPLAYSTRINGHEAD above
		TLKEntry* tlkEntry = IDTable::GetTLKEntry(params->integer1);
		owner->SetText(tlkEntry->text);
		delete tlkEntry;
	}
	if (state.counter-- <= 0) {
		owner->SetText("");
		state.completed = true;
	}
}


// SetAreaRestFlag(I:CanRest*) - stateless. See AreaRoom::SetCanRest()'s
// header comment and REST()/RESTPARTY() above, which now consult it.
static void
RunActionSetAreaRestFlag(Object* sender, action_params* params, action_state& state)
{
	AreaRoom* area = dynamic_cast<AreaRoom*>(Core::Get()->CurrentRoom());
	if (area != NULL)
		area->SetCanRest(params->integer1 != 0);
	state.completed = true;
}


// RevealAreaOnMap(S:ResRef*) / HideAreaOnMap(S:ResRef*) - stateless.
// Overrides the area's own file-driven worldmap visibility bit (see
// Game::SetAreaMapVisible(), AreaEntry::IsVisible()/SetVisible() in
// resources/WMAPResource.h, and WorldMap::_LoadAreaEntries(), which
// applies the override on top of the file's own flag). Only the icon/
// name display is gated this way - travel isn't (WorldMap::MouseDown()
// doesn't check any flag before allowing it either), so this doesn't
// model the "Reachable" bit's actual travel-blocking.
static void
RunActionRevealAreaOnMap(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->SetAreaMapVisible(params->string1, true);
	state.completed = true;
}


static void
RunActionHideAreaOnMap(Object* sender, action_params* params, action_state& state)
{
	Game::Get()->SetAreaMapVisible(params->string1, false);
	state.completed = true;
}


// PlayDeadInterruptible(I:Time*) - state: plays the DEAD/SLEEP animation
// (Animation.h's ACT_DEAD) for integer1 ticks. The "interruptible by a
// standard PC move command" nuance isn't modeled (this engine's action
// state machine has no generic way for an external command to cancel an
// in-progress action) - it just runs for the full duration.
static void
RunActionPlayDeadInterruptible(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		actor->SetAnimationAction(ACT_DEAD);
		state.counter = params->integer1;
		state.initiated = true;
	}

	if (state.counter-- <= 0) {
		actor->SetAnimationAction(ACT_STANDING);
		state.completed = true;
	}
}


// MoveToCenterOfScreen(I:NotInterruptableFor*) - state: same walk-to-
// point pattern as MOVETOPOINT, targeting RoomBase::AreaCenterPoint()
// (the world-coordinate center of the current viewport - already used
// for view-centering elsewhere in this engine). The "script conditions
// not checked for the duration" nuance isn't modeled.
static void
RunActionMoveToCenterOfScreen(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor == NULL) {
		state.completed = true;
		return;
	}

	if (!state.initiated) {
		AreaRoom* area = actor->Area();
		if (area != NULL)
			actor->SetDestination(area->AreaCenterPoint());
		state.initiated = true;
	}

	if (!actor->MoveToNextPointInPath(false))
		state.completed = true;
}


// MoveViewObject(O:Target*,I:ScrollSpeed*Scroll) - same destination-
// tracking pattern as MOVEVIEWPOINT right above (state.point/extra),
// just resolving the target object's position once at the start
// instead of taking P:Target* directly.
static void
RunActionMoveViewObject(Object* sender, action_params* params, action_state& state)
{
	if (!state.initiated) {
		Object* target = Script::GetTargetObject(sender, params);
		if (target == nullptr) {
			state.completed = true;
			return;
		}

		state.initiated = true;
		state.point = _ObjectPosition(target);
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


// DayNight(I:TimeOfDay*Time) - stateless. Per IESDP the time value comes
// from TIME.IDS, which (per the game's own Time.ids file) is just a
// plain 0-23 hour-of-day number (e.g. 0=MIDNIGHT, 12=NOON) - advances
// the clock forward to the next occurrence of that hour, then reloads
// the area for its day/night WED graphics, same mechanism
// Game::ToggleDayNight() already uses for its own fixed +12h jump.
static void
RunActionDayNight(Object* sender, action_params* params, action_state& state)
{
	uint16 targetHour = (uint16)(params->integer1 % 24);
	uint16 currentHour = GameTimer::HourOfDay();
	uint16 delta = (targetHour + 24 - currentHour) % 24;
	GameTimer::AdvanceTime(delta, 0, 0);

	RoomBase* area = Core::Get()->CurrentRoom();
	if (area != NULL)
		area->ReloadArea();
	state.completed = true;
}


static bool
_IsMeleeAttackType(uint8 attackType)
{
	return attackType == 1; // itm_ability::attackType: 1 = Melee
}


static bool
_IsRangedAttackType(uint8 attackType)
{
	return attackType == 2 || attackType == 4; // Projectile or Launcher
}


// Shared by EquipMostDamagingMelee/EquipRanged below. IESDP describes
// both as picking among weapons "available in the quickslots" (BG2's 4
// pre-equipped, switchable weapon slots) - this engine only has a
// single weapon slot (kSlotWeaponFirst - see Actor::EquippedWeapon(),
// which always reads that one slot), so both scan the whole inventory
// instead and equip the match into that slot via Actor::EquipItem(),
// same as any other equip action here. Only the primary ability (index
// 0) of each item is examined, same convention Actor::AttackTarget()
// already uses to read a weapon's attack. Returns an empty res_ref if
// nothing matched.
static res_ref
_FindBestWeapon(Actor* actor, bool (*matchesType)(uint8), bool compareDamage)
{
	res_ref best;
	int32 bestScore = -1;
	for (uint32 slot = 0; slot < kNumItemSlots; slot++) {
		IE::item item;
		if (!actor->CRE()->GetItemAtSlot(slot, item) || item.name.name[0] == '\0')
			continue;

		ITMResource* itm = gResManager->GetITM(item.name);
		if (itm == NULL)
			continue;

		itm_ability ability;
		bool hasAbility = itm->GetAbility(0, ability);
		gResManager->ReleaseResource(itm);
		if (!hasAbility || !matchesType(ability.attackType))
			continue;

		if (!compareDamage)
			return item.name;

		// Per IESDP "damage is calculated on the THAC0 bonus and
		// damage" - combined into one score (average damage roll +
		// damage bonus + THAC0 bonus, higher is better); special
		// bonuses vs. creature types/elemental damage aren't checked,
		// matching IESDP's own note that the real engine doesn't
		// either.
		int32 score = ability.diceThrown * (ability.diceSides + 1) / 2
			+ ability.damageBonus + ability.thac0Bonus;
		if (score > bestScore) {
			bestScore = score;
			best = item.name;
		}
	}
	return best;
}


// EquipMostDamagingMelee() - stateless.
static void
RunActionEquipMostDamagingMelee(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		res_ref weapon = _FindBestWeapon(actor, _IsMeleeAttackType, true);
		if (weapon.name[0] != '\0')
			actor->EquipItem(weapon);
	}
	state.completed = true;
}


// EquipRanged() - stateless. Per IESDP just picks a ranged weapon,
// no damage comparison (unlike EquipMostDamagingMelee above) - first
// match wins.
static void
RunActionEquipRanged(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL) {
		res_ref weapon = _FindBestWeapon(actor, _IsRangedAttackType, false);
		if (weapon.name[0] != '\0')
			actor->EquipItem(weapon);
	}
	state.completed = true;
}


// RandomTurn() - stateless. Faces a random direction (0-15, BG2's
// 16-orientation scheme).
static void
RunActionRandomTurn(Object* sender, action_params* params, action_state& state)
{
	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	if (actor != NULL)
		actor->SetOrientation(Core::RandomNumber(0, 15));
	state.completed = true;
}


// UseContainer() - stateless. Per IESDP "used by the engine internally"
// (queued when the player clicks a container - see Actor::ClickedOn(),
// which now does exactly that, after a MOVETOOBJECT to reach it first).
// Logs the container's contents (Container::ItemCount()/ItemAt(), read
// from the area's shared item list - see ARAResource::GetContainerAt())
// rather than moving them into the party's inventory - no loot GUI
// exists yet (see the Fase 6 plan notes), and the user explicitly chose
// "walk there + log" over auto-loot for this pass.
static void
RunActionUseContainer(Object* sender, action_params* params, action_state& state)
{
	state.completed = true;

	Actor* actor = dynamic_cast<Actor*>(Script::GetSenderObject(sender, params));
	Container* container = dynamic_cast<Container*>(Script::GetTargetObject(sender, params));
	if (actor == NULL || actor->CRE() == NULL || container == NULL)
		return;
	if (!container->IsEnabled())
		return;

	// Auto-loot (no loot GUI): take everything that fits into the
	// creature's inventory, leave the rest in the container. Session-only
	// - the container's remaining contents are cached across area
	// re-entry (Game::AreaCache), not written to a savegame.
	for (uint32 i = 0; i < container->ItemCount(); ) {
		const IE::item& item = container->ItemAt(i);
		if (actor->AddItem(item.name, item.quantity1 > 0 ? item.quantity1 : 1)) {
			IE::item taken;
			container->TakeItemAt(i, taken);
			std::cout << actor->Name() << " takes " << taken.name.CString()
					<< " from " << container->Name() << std::endl;
		} else {
			i++; // no room - leave it
		}
	}
}



static const ActionDescriptor kActionsTable[] = {
		{ 0, "NOACTION", NULL },
		{ 1, "ACTIONOVERRIDE", NULL },
		{ 2, "ADDWAYPOINT", NULL },
		{ 3, "ATTACK", RunActionAttack },
		{ 5, "BACKSTAB", NULL },
		{ 7, "CREATECREATURE", RunActionCreateCreature },
		{ 8, "DIALOG", RunActionDialog },
		{ 9, "DROPITEM", RunActionDropItem },
		{ 10, "ENEMY", RunActionSetEnemyAlly },
		{ 11, "EQUIPITEM", RunActionEquipItem },
		{ 13, "FINDTRAPS", RunActionFindTraps },
		{ 14, "GETITEM", RunActionGetItem },
		{ 15, "GIVEITEM", RunActionGiveItem },
		{ 16, "GIVEORDER", NULL },
		{ 17, "HELP", NULL },
		{ 18, "HIDE", NULL },
		{ 19, "JOINPARTY", RunActionJoinParty },
		{ 20, "LAYHANDS", NULL },
		{ 21, "LEAVEPARTY", RunActionLeaveParty },
		{ 22, "MOVETOOBJECT", RunActionWalkToObject },
		{ 23, "MOVETOPOINT", RunActionWalkTo },
		{ 24, "PANIC", RunActionRandomWalk },
		{ 25, "PICKPOCKETS", NULL },
		{ 26, "PLAYSOUND", RunActionPlaySound },
		{ 27, "PROTECTPOINT", NULL },
		{ 28, "REMOVETRAPS", RunActionRemoveTraps },
		{ 29, "RUNAWAYFROM", RunActionRunAwayFrom },
		{ 30, "SETGLOBAL", RunActionSetGlobal },
		{ 31, "SPELL", RunActionSpell },
		{ 33, "TURN", NULL },
		{ 34, "USEITEMSLOT", RunActionUseItemSlot },
		{ 36, "CONTINUE", NULL },
		{ 37, "FOLLOWPATH", NULL },
		{ 38, "SWING", NULL },
		{ 39, "RECOIL", NULL },
		{ 40, "PLAYDEAD", RunActionPlayDead },
		{ 47, "FORMATION", NULL },
		{ 48, "JUMPTOPOINT", RunActionJumpToPoint },
		{ 49, "MOVEVIEWPOINT", RunActionMoveViewPoint },
		{ 50, "MOVEVIEWOBJECT", RunActionMoveViewObject },
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
		{ 66, "DAYNIGHT", RunActionDayNight },
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
		{ 82, "CREATEITEM", RunActionCreateItem },
		{ 83, "SMALLWAIT", RunActionSmallWait },
		{ 84, "FACE", RunActionChangeOrientationExt },
		{ 85, "RANDOMWALK", RunActionRandomWalk },
		{ 86, "SETINTERRUPT", RunActionSetInterruptable },
		{ 87, "PROTECTOBJECT", NULL },
		{ 88, "LEADER", NULL },
		{ 89, "FOLLOW", NULL },
		{ 90, "MOVETOPOINTNORECTICLE", RunActionWalkTo },
		{ 91, "LEAVEAREA", NULL },
		{ 92, "SELECTWEAPONABILITY", NULL },
		{ 94, "GROUPATTACK", NULL },
		// SPELLPOINT: same shape as SPELL (consumes a memorized slot),
		// just targeting a point instead of an object - like
		// FORCESPELLPOINT above, point-targeting isn't modeled
		// separately from object-targeting, so this is a direct alias.
		{ 95, "SPELLPOINT", RunActionSpell },
		{ 96, "REST", RunActionRest },
		{ 97, "USEITEMPOINTSLOT", NULL },
		// AttackNoSound/AttackOneRound: same as ATTACK per IESDP, just
		// without a battlecry sound / limited to one round - neither
		// nuance is modeled (no attack sounds, no round-limiting
		// mechanism), same alias already used for ATTACKREEVALUATE.
		{ 98, "ATTACKNOSOUND", RunActionAttack },
		{ 100, "RANDOMFLY", RunActionRandomFly },
		{ 101, "FLYTOPOINT", RunActionFlyTo }, // not in the original IDS table
		{ 102, "MORALESET", RunActionMoraleSet },
		{ 103, "MORALEINC", RunActionMoraleInc },
		{ 104, "MORALEDEC", RunActionMoraleDec },
		{ 105, "ATTACKONEROUND", RunActionAttack },
		{ 106, "SHOUT", RunActionShout },
		{ 107, "MOVETOOFFSET", RunActionMoveToOffset },
		{ 108, "ESCAPEAREA", RunActionEscapeArea },
		{ 108, "ESCAPEAREAMOVE", RunActionEscapeArea },
		{ 109, "INCREMENTGLOBAL", RunActionIncrementGlobal },
		{ 110, "LEAVEAREALUA", RunActionChangeArea },
		{ 111, "DESTROYSELF", RunActionDestroySelf },
		{ 112, "USECONTAINER", RunActionUseContainer },
		{ 113, "FORCESPELL", RunActionForceSpell },
		{ 114, "FORCESPELLPOINT", RunActionForceSpellPoint },
		{ 115, "SETGLOBALTIMER", RunActionSetGlobalTimer },
		{ 116, "TAKEPARTYITEM", RunActionTakePartyItem },
		{ 117, "TAKEPARTYGOLD", RunActionTakePartyGold },
		{ 118, "GIVEPARTYGOLD", RunActionGivePartyGold },
		{ 119, "DROPINVENTORY", RunActionClearInventory },
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
		{ 130, "RANDOMTURN", RunActionRandomTurn },
		{ 131, "KILL", RunActionKill },
		{ 132, "VERBALCONSTANT", RunActionVerbalConstant },
		{ 133, "CLEARACTIONS", RunActionClearActions },
		{ 134, "ATTACKREEVALUATE", RunActionAttack },
		{ 135, "LOCKSCROLL", NULL },
		{ 136, "UNLOCKSCROLL", NULL },
		{ 137, "STARTDIALOGUE", RunActionStartDialogue },
		{ 138, "SETDIALOGUE", RunActionSetDialogue },
		{ 139, "PLAYERDIALOGUE", RunActionStartDialogue },
		{ 140, "GIVEITEMCREATE", RunActionGiveItemCreate },
		{ 141, "GIVEPARTYGOLDGLOBAL", RunActionGivePartyGoldGlobal },
		{ 142, "USEDOOR", NULL },
		{ 143, "OPENDOOR", RunActionOpenDoor },
		{ 144, "CLOSEDOOR", RunActionCloseDoor },
		{ 145, "PICKLOCK", RunActionPickLock },
		{ 146, "POLYMORPH", NULL },
		{ 147, "REMOVESPELL", RunActionRemoveSpell },
		{ 148, "BASHDOOR", RunActionBashDoor },
		{ 149, "EQUIPMOSTDAMAGINGMELEE", RunActionEquipMostDamagingMelee },
		{ 150, "STARTSTORE", RunActionStartStore },
		{ 151, "DISPLAYSTRING", RunActionDisplayMessage },
		{ 152, "CHANGEAITYPE", NULL },
		{ 153, "CHANGEENEMYALLY", RunActionChangeEnemyAlly },
		{ 154, "CHANGEGENERAL", RunActionChangeGeneral },
		{ 155, "CHANGERACE", RunActionChangeRace },
		{ 156, "CHANGECLASS", RunActionChangeClass },
		{ 157, "CHANGESPECIFICS", RunActionChangeSpecifics },
		{ 158, "CHANGEGENDER", RunActionChangeGender },
		{ 159, "CHANGEALIGNMENT", RunActionChangeAlignment },
		{ 160, "APPLYSPELL", RunActionApplySpell },
		{ 161, "INCREMENTCHAPTER", RunActionIncrementChapter },
		{ 162, "REPUTATIONSET", RunActionReputationSet },
		{ 163, "REPUTATIONINC", RunActionReputationInc },
		{ 164, "ADDEXPERIENCEPARTY", RunActionAddExperienceParty },
		{ 165, "ADDEXPERIENCEPARTYGLOBAL", RunActionAddExperiencePartyGlobal },
		{ 166, "SETNUMTIMESTALKEDTO", RunActionSetNumTimesTalkedTo },
		{ 167, "STARTMOVIE", RunActionPlayMovie },
		{ 168, "INTERACT", NULL },
		{ 169, "DESTROYITEM", RunActionDestroyItem },
		{ 170, "REVEALAREAONMAP", RunActionRevealAreaOnMap },
		{ 171, "GIVEGOLDFORCE", RunActionGivePartyGold },
		{ 172, "CHANGETILESTATE", NULL },
		{ 173, "ADDJOURNALENTRY", RunActionAddJournalEntry },
		{ 174, "EQUIPRANGED", RunActionEquipRanged },
		{ 175, "SETLEAVEPARTYDIALOGUEFILE", NULL },
		{ 176, "ESCAPEAREADESTROY", RunActionEscapeArea },
		{ 177, "TRIGGERACTIVATION", RunActionTriggerActivation },
		{ 178, "BREAKINSTANTS", NULL },
		{ 179, "DIALOGUEINTERRUPT", NULL },
		{ 180, "MOVETOOBJECTFOLLOW", NULL },
		{ 181, "REALLYFORCESPELL", RunActionForceSpell },
		{ 182, "MAKEUNSELECTABLE", NULL },
		{ 183, "MULTIPLAYERSYNC", NULL },
		{ 184, "RUNAWAYFROMNOINTERRUPT", NULL },
		{ 185, "SETMASTERAREA", NULL },
		{ 186, "ENDCREDITS", NULL },
		{ 187, "STARTMUSIC", NULL },
		{ 188, "TAKEPARTYITEMALL", RunActionTakePartyItemAll },
		{ 189, "LEAVEAREALUAPANIC", NULL },
		{ 190, "SAVEGAME", RunActionSaveGame },
		// SpellNoDec/SpellPointNoDec: cast without spending a memorized
		// slot - exactly what ForceSpell/ForceSpellPoint already do
		// (neither checks/consumes the spellbook), so these are direct
		// aliases; IESDP's own text for 192 ("must currently be
		// memorised") looks like a documentation slip against the
		// "NoDec" name shared with 191 - not modeled either way.
		{ 191, "SPELLNODEC", RunActionForceSpell },
		{ 192, "SPELLPOINTNODEC", RunActionForceSpellPoint },
		{ 193, "TAKEPARTYITEMRANGE", RunActionTakePartyItem },
		{ 194, "CHANGEANIMATION", NULL },
		{ 195, "LOCK", RunActionLock },
		{ 196, "UNLOCK", RunActionUnlock },
		{ 197, "MOVEGLOBAL", NULL },
		{ 198, "STARTDIALOGNOSET", RunActionDialog },
		{ 199, "TEXTSCREEN", NULL },
		{ 200, "RANDOMWALKCONTINUOUS", NULL },
		{ 201, "DETECTSECRETDOOR", RunActionDetectSecretDoor },
		{ 202, "FADETOCOLOR", RunActionFadeToColor },
		{ 203, "FADEFROMCOLOR", RunActionFadeFromColor },
		{ 204, "TAKEPARTYITEMNUM", RunActionTakePartyItemNum },
		{ 207, "MOVETOPOINTNOINTERRUPT", RunActionWalkTo },
		{ 208, "MOVETOOBJECTNOINTERRUPT", RunActionWalkToObject },
		{ 209, "SPAWNPTACTIVATE", NULL },
		{ 210, "SPAWNPTDEACTIVATE", NULL },
		{ 211, "SPAWNPTSPAWN", NULL },
		{ 212, "GLOBALSHOUT", NULL },
		{ 213, "STATICSTART", NULL },
		{ 214, "STATICSTOP", NULL },
		{ 215, "FOLLOWOBJECTFORMATION", NULL },
		{ 216, "ADDFAMILIAR", NULL },
		{ 217, "REMOVEFAMILIAR", NULL },
		{ 218, "PAUSEGAME", RunActionPauseGame },
		{ 219, "CHANGEANIMATIONNOEFFECT", NULL },
		{ 220, "TAKEITEMLISTPARTY", RunActionTakeItemListParty },
		{ 221, "SETMORALEAI", NULL },
		{ 222, "INCMORALEAI", NULL },
		{ 223, "DESTROYALLEQUIPMENT", RunActionClearInventory },
		{ 224, "GIVEPARTYALLEQUIPMENT", RunActionGivePartyAllEquipment },
		{ 225, "MOVEBETWEENAREASEFFECT", RunActionMoveBetweenAreasEffect },
		{ 226, "TAKEITEMLISTPARTYNUM", RunActionTakeItemListPartyNum },
		{ 227, "CREATECREATUREOBJECTEFFECT", RunActionCreateCreatureNearObject },
		{ 228, "CREATECREATUREIMPASSABLE", RunActionCreateCreatureImpassable },
		{ 229, "FACEOBJECT", RunActionFaceObject },
		{ 230, "RESTPARTY", RunActionRestParty },
		{ 231, "CREATECREATUREDOOR", RunActionCreateCreature },
		{ 232, "CREATECREATUREOBJECTDOOR", RunActionCreateCreatureNearObject },
		{ 233, "CREATECREATUREOBJECTOFFSCREEN", RunActionCreateCreatureNearObject },
		{ 234, "MOVEGLOBALOBJECTOFFSCREEN", NULL },
		{ 235, "SETQUESTDONE", RunActionEraseJournalEntry },
		{ 236, "STOREPARTYLOCATIONS", RunActionStorePartyLocations },
		{ 237, "RESTOREPARTYLOCATIONS", RunActionRestorePartyLocations },
		{ 238, "CREATECREATUREOFFSCREEN", RunActionCreateCreature },
		{ 239, "MOVETOCENTEROFSCREEN", RunActionMoveToCenterOfScreen },
		{ 240, "REALLYFORCESPELLDEAD", RunActionForceSpell },
		{ 241, "CALM", RunActionCalm },
		{ 242, "ALLY", RunActionAlly },
		{ 243, "RESTNOSPELLS", RunActionRestParty },
		{ 244, "SAVELOCATION", RunActionSaveLocation },
		{ 245, "SAVEOBJECTLOCATION", RunActionSaveObjectLocation },
		{ 246, "CREATECREATUREATLOCATION", NULL },
		{ 247, "SETTOKEN", RunActionSetToken },
		{ 248, "SETTOKENOBJECT", RunActionSetTokenObject },
		{ 249, "SETGABBER", RunActionSetGabber },
		{ 250, "CREATECREATUREOBJECTCOPYEFFECT", RunActionCreateCreatureNearObject },
		{ 251, "HIDEAREAONMAP", RunActionHideAreaOnMap },
		{ 252, "CREATECREATUREOBJECTOFFSET", RunActionCreateCreatureObjectOffset },
		{ 253, "CONTAINERENABLE", RunActionContainerEnable },
		{ 254, "SCREENSHAKE", RunActionScreenShake },
		{ 255, "ADDGLOBALS", RunActionAddGlobals },
		{ 256, "CREATEITEMGLOBAL", NULL },
		{ 257, "PICKUPITEM", NULL },
		{ 258, "FILLSLOT", NULL },
		{ 259, "ADDXPOBJECT", RunActionAddXPObject },
		{ 260, "DESTROYGOLD", RunActionDestroyGold },
		{ 261, "SETHOMELOCATION", RunActionSetHomeLocation },
		{ 262, "DISPLAYSTRINGNONAME", RunActionDisplayMessage },
		{ 263, "ERASEJOURNALENTRY", RunActionEraseJournalEntry },
		{ 264, "COPYGROUNDPILESTO", NULL },
		{ 265, "DIALOGFORCEINTERRUPT", RunActionDialogForceInterrupt },
		{ 266, "STARTDIALOGUEINTERRUPT", RunActionStartDialogueInterrupt },
		{ 267, "STARTDIALOGNOSETINTERRUPT", RunActionStartDialogueInterrupt },
		{ 268, "REALSETGLOBALTIMER", RunActionRealSetGlobalTimer },
		{ 269, "DISPLAYSTRINGHEAD", RunActionDisplayStringHead },
		{ 270, "POLYMORPHCOPY", NULL },
		{ 271, "VERBALCONSTANTHEAD", NULL },
		{ 272, "CREATEVISUALEFFECT", RunActionCreateVisualEffect },
		{ 273, "CREATEVISUALEFFECTOBJECT", RunActionCreateVisualEffectObject },
		{ 274, "ADDKIT", RunActionAddKit },
		{ 275, "STARTCOMBATCOUNTER", NULL },
		{ 276, "ESCAPEAREANOSEE", RunActionEscapeArea },
		{ 277, "ESCAPEAREAOBJECTMOVE", RunActionEscapeAreaObjectMove },
		{ 278, "TAKEITEMREPLACE", RunActionTakeItemReplace },
		{ 279, "ADDSPECIALABILITY", NULL },
		{ 280, "DESTROYALLDESTRUCTABLEEQUIPMENT", RunActionClearInventory },
		{ 281, "REMOVEPALADINHOOD", NULL },
		{ 282, "REMOVERANGERHOOD", NULL },
		{ 283, "REGAINPALADINHOOD", NULL },
		{ 284, "REGAINRANGERHOOD", NULL },
		{ 285, "POLYMORPHCOPYBASE", NULL },
		{ 286, "HIDEGUI", RunActionHideGUI },
		{ 287, "UNHIDEGUI", RunActionUnhideGUI },
		{ 288, "SETNAME", RunActionSetName },
		{ 289, "ADDSUPERKIT", NULL },
		{ 290, "PLAYDEADINTERRUPTIBLE", RunActionPlayDeadInterruptible },
		{ 291, "MOVEGLOBALOBJECT", NULL },
		{ 292, "DISPLAYSTRINGHEADOWNER", RunActionDisplayStringHeadOwner },
		{ 293, "STARTDIALOGOVERRIDE", RunActionStartDialogue },
		{ 294, "STARTDIALOGOVERRIDEINTERRUPT", RunActionStartDialogueInterrupt },
		{ 295, "CREATECREATURECOPYPOINT", RunActionCreateCreature },
		{ 296, "BATTLESONG", NULL },
		{ 297, "MOVETOSAVEDLOCATIONN", RunActionMoveToSavedLocation },
		{ 298, "APPLYDAMAGE", RunActionApplyDamage },
		{ 299, "BANTERBLOCKTIME", NULL },
		{ 300, "BANTERBLOCKFLAG", NULL },
		{ 301, "AMBIENTACTIVATE", NULL },
		{ 302, "ATTACHTRANSITIONTODOOR", NULL },
		{ 303, "DEATHMATCHPOSITIONGLOBAL", NULL },
		{ 304, "DEATHMATCHPOSITIONAREA", NULL },
		{ 305, "DEATHMATCHPOSITIONLOCAL", NULL },
		{ 306, "APPLYDAMAGEPERCENT", RunActionApplyDamagePercent },
		{ 307, "SG", RunActionSG },
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
		{ 318, "FORCESPELLRANGE", RunActionForceSpell },
		{ 319, "FORCESPELLPOINTRANGE", RunActionForceSpellPoint },
		{ 320, "SETPLAYERSOUND", NULL },
		{ 321, "SETAREARESTFLAG", RunActionSetAreaRestFlag },
		{ 322, "FAKEEFFECTEXPIRYCHECK", NULL },
		{ 323, "CREATECREATUREIMPASSABLEALLOWOVERLAP", RunActionCreateCreatureImpassable },
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

