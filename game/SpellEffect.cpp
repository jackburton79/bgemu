/*
 * SpellEffect.cpp
 *
 *  Created on: 7 gen 2022
 *      Author: Jackburton
 */

#include "SpellEffect.h"

#include "Actor.h"
#include "AreaRoom.h"
#include "Core.h"
#include "CreResource.h"
#include "Effect.h"
#include "Log.h"
#include "Object.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>


SpellEffect::SpellEffect(int16 opcode, Object* source, int32 parameter1,
		int32 parameter2, uint32 duration, const std::string& resource,
		uint32 savingThrowType, int32 savingThrowBonus)
	:
	fOpcode(opcode),
	fSource(source),
	fParameter1(parameter1),
	fParameter2(parameter2),
	fDuration(duration),
	fPermanent(duration == 0),
	fInitiated(false),
	fResource(resource),
	fSavingThrowType(savingThrowType),
	fSavingThrowBonus(savingThrowBonus)
{
	// Mirrors Action's own pattern: hold a reference to the source object
	// for as long as this effect is alive, in case it's destroyed (e.g.
	// the caster dies) while the effect is still lingering on its target.
	if (fSource != NULL)
		fSource->Acquire();
}


SpellEffect::~SpellEffect()
{
	if (fSource != NULL)
		fSource->Release();
}


int16
SpellEffect::Opcode() const
{
	return fOpcode;
}


Object*
SpellEffect::Source() const
{
	return fSource;
}


int32
SpellEffect::Parameter1() const
{
	return fParameter1;
}


int32
SpellEffect::Parameter2() const
{
	return fParameter2;
}


const std::string&
SpellEffect::Resource() const
{
	return fResource;
}


uint32
SpellEffect::SavingThrowType() const
{
	return fSavingThrowType;
}


int32
SpellEffect::SavingThrowBonus() const
{
	return fSavingThrowBonus;
}


bool
SpellEffect::Initiated() const
{
	return fInitiated;
}


void
SpellEffect::SetInitiated()
{
	fInitiated = true;
}


bool
SpellEffect::Tick()
{
	if (fPermanent)
		return false;

	if (fDuration > 0)
		fDuration--;

	return fDuration == 0;
}


std::string
SpellEffect::Name() const
{
	return fResource.empty() ? std::to_string(fOpcode) : fResource;
}



// Rolls the saving throw an effect requests (bit0 Spells, bit1 Breath,
// bit2 Death, bit3 Wands, bit4 Polymorph - see IESDP spl_v1 feature
// block), against the best (lowest, i.e. easiest to make) of the
// requested SaveVersus fields. Returns true if the target SAVED
// (the effect should be negated/skipped), false if it failed (or the
// effect requests no save at all, savingThrowType == 0 - matching this
// codebase's existing "no saves implemented" behavior for effects that
// don't opt in).
static bool
_RollSave(Actor* target, SpellEffect& effect)
{
	uint32 type = effect.SavingThrowType();
	if (type == 0)
		return false;

	SaveVersus saves = target->CRE()->Saves();
	int32 best = 255;
	if (type & 0x01)
		best = std::min(best, (int32)saves.spell);
	if (type & 0x02)
		best = std::min(best, (int32)saves.breath);
	if (type & 0x04)
		best = std::min(best, (int32)saves.death);
	if (type & 0x08)
		best = std::min(best, (int32)saves.wands);
	if (type & 0x10)
		best = std::min(best, (int32)saves.poly);
	if (best == 255)
		return false;

	const int32 neededRoll = best - effect.SavingThrowBonus();
	const int32 roll = Core::RollDice(1, 20, 0);
	return roll >= neededRoll;
}


// Shared by every "impose a STATE.IDS bit for the effect's duration"
// opcode below. On the first tick, rolls a save (if requested) and either
// negates the effect (removed immediately) or sets the bit; on later
// ticks it's a no-op (the bit stays set, duration counts down as usual
// via SpellEffect::Tick() in Object::_ApplySpellEffects()).
static bool
_ApplyStateEffect(Object* target, SpellEffect& effect, uint32 stateBit)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	if (!effect.Initiated()) {
		effect.SetInitiated();
		if (_RollSave(actor, effect))
			return true; // saved: effect negated, remove immediately

		CREResource* cre = actor->CRE();
		cre->SetPermanentStatus(cre->PermanentStatus() | stateBit);
	}

	return false; // stays active; cleanup (below) clears the bit on expiry
}


// The `cleanup` counterpart to _ApplyStateEffect(): called once, right
// before the effect is removed (see EffectDescriptor::cleanup).
static void
_ClearStateEffect(Object* target, SpellEffect& effect, uint32 stateBit)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return;

	CREResource* cre = actor->CRE();
	cre->SetPermanentStatus(cre->PermanentStatus() & ~stateBit);
}


// Generates a RunEffect<Name>/CleanupEffect<Name> pair around
// _ApplyStateEffect()/_ClearStateEffect() for a single STATE.IDS bit -
// used for every opcode whose only job is "impose this state for the
// effect's duration".
#define DEFINE_STATE_EFFECT(nameSuffix, stateBit) \
	static bool RunEffect##nameSuffix(Object* target, SpellEffect& effect) { \
		return _ApplyStateEffect(target, effect, stateBit); \
	} \
	static void CleanupEffect##nameSuffix(Object* target, SpellEffect& effect) { \
		_ClearStateEffect(target, effect, stateBit); \
	}

// Generates a one-shot RunEffectCure<Name> handler that just clears the
// bit immediately - for opcodes with a paired "Cure: X" effect (only some
// states have one in BG2's opcode list).
#define DEFINE_CURE_EFFECT(nameSuffix, stateBit) \
	static bool RunEffectCure##nameSuffix(Object* target, SpellEffect& effect) { \
		_ClearStateEffect(target, effect, stateBit); \
		return true; \
	}

DEFINE_STATE_EFFECT(Invisibility, STATE_INVISIBLE)
DEFINE_STATE_EFFECT(Silence, STATE_SILENCED)
DEFINE_STATE_EFFECT(Stun, STATE_STUNNED)
DEFINE_STATE_EFFECT(Blindness, STATE_BLIND)
DEFINE_STATE_EFFECT(Berserk, STATE_BERSERK)
DEFINE_STATE_EFFECT(Panic, STATE_PANIC)
DEFINE_STATE_EFFECT(Sleep, STATE_SLEEPING)
DEFINE_STATE_EFFECT(Slow, STATE_SLOWED)
DEFINE_STATE_EFFECT(Hold, STATE_HELPLESS)
DEFINE_STATE_EFFECT(Confusion, STATE_CONFUSED)
DEFINE_STATE_EFFECT(Haste, STATE_HASTED)
DEFINE_STATE_EFFECT(Poison, STATE_POISONED) // bit only - no periodic damage yet

#undef DEFINE_STATE_EFFECT

DEFINE_CURE_EFFECT(Sleep, STATE_SLEEPING)
DEFINE_CURE_EFFECT(Berserk, STATE_BERSERK)
DEFINE_CURE_EFFECT(Hold, STATE_HELPLESS)
DEFINE_CURE_EFFECT(Confusion, STATE_CONFUSED)

#undef DEFINE_CURE_EFFECT


// #13 "Death: Instant Death" - same unconditional-kill mechanics as #238
// Disintegrate, but (unlike Disintegrate, per IESDP) this one allows a
// saving throw.
static bool
RunEffectInstantDeath(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	if (_RollSave(actor, effect))
		return true; // saved: no effect

	actor->ApplyDamage(actor->CRE()->CurrentHitPoints());
	return true; // one-shot: remove immediately once applied
}


// #0 "Stat: AC vs. Damage Type Modifier". Parameter1 is the AC delta
// (negative improves AC, matching AD&D 2e convention), Parameter2 is a
// type bitmask (0=all, 1=Crushing, 2=Missile, 4=Piercing, 8=Slashing).
// The "16 = Base AC setting" special case isn't implemented (logged and
// dropped, like HP:Damage's unsupported modes) - it would need to know
// the pre-effect AC to restore on cleanup, which this delta-based
// apply/undo doesn't track.
static bool
RunEffectACBonus(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	if (effect.Parameter2() == 16) {
		std::cerr << Log::Red << target->Name()
				<< ": Stat: AC Modifier \"Base AC setting\" not implemented"
				<< Log::Normal << std::endl;
		return true;
	}

	if (!effect.Initiated()) {
		effect.SetInitiated();
		if (_RollSave(actor, effect))
			return true;
		actor->CRE()->ModifyAC((int16)effect.Parameter1(), (uint8)effect.Parameter2());
	}

	return false;
}


static void
CleanupEffectACBonus(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return;
	actor->CRE()->ModifyAC(-(int16)effect.Parameter1(), (uint8)effect.Parameter2());
}


// #54 "Stat: THAC0 Modifier". Only Type 0 ("Cumulative Modifier",
// Parameter2 == 0) is implemented; Flat Value (1) and Percentage (2) are
// logged and dropped, same precedent as HP:Damage's unsupported modes.
static bool
RunEffectTHAC0Bonus(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	if (effect.Parameter2() != 0) {
		std::cerr << Log::Red << target->Name()
				<< ": Stat: THAC0 Modifier type " << effect.Parameter2()
				<< " not implemented" << Log::Normal << std::endl;
		return true;
	}

	if (!effect.Initiated()) {
		effect.SetInitiated();
		if (_RollSave(actor, effect))
			return true;
		actor->CRE()->ModifyTHAC0((int8)effect.Parameter1());
	}

	return false;
}


static void
CleanupEffectTHAC0Bonus(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return;
	actor->CRE()->ModifyTHAC0(-(int8)effect.Parameter1());
}


// #124 "Spell Effect: Teleport (Dimension Door)" and #274 "Teleport to
// Target" - same one-shot "move the target elsewhere" effect (IESDP gives
// them near-identical descriptions, and neither carries a real destination
// in its parameters), so they share this handler.
static bool
RunEffectTeleportToTarget(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	// TODO: Use a real destination instead of hardcoding the map origin.
	IE::point home = {0, 0};
	actor->SetPosition(home);

	return true; // one-shot: remove immediately once applied
}


// #12 "HP: Damage". Parameter1 is the damage amount; Parameter2 packs a
// damage type in its high 16 bits and an application mode (0: subtract
// amount, 1: set to value, 2: set to percentage, 3: reduce by percentage)
// in its low 16 bits - see IESDP opcode #12. Only mode 0 (plain damage) is
// implemented, which is what the opening BG2 cutscene's CUTSCENE_DAMAGE_1(B)
// spells use; other modes are logged and dropped rather than risk applying
// the wrong amount.
static bool
RunEffectHPDamage(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	int32 mode = effect.Parameter2() & 0xFFFF;
	if (mode != 0) {
		std::cerr << Log::Red << target->Name()
				<< ": HP: Damage mode " << mode << " not implemented"
				<< Log::Normal << std::endl;
		return true;
	}

	actor->ApplyDamage(effect.Parameter1());

	return true; // one-shot: remove immediately once applied
}


// #215 "Graphics: Play 3D Effect". Parameter2 is the "Effect State":
// 0 - play on target, not attached; 1 - play on target, attached;
// 2 - play on point. This engine's Effect class (game/Effect.h) is only
// ever a one-shot animation at a fixed point - there's no notion of an
// effect staying "attached" to a moving actor - so states 0 and 1 are
// both handled the same way: spawn it once at the target's current
// position, mirroring RunActionCreateVisualEffectObject() in Actions.cpp.
// State 2 ("on point") would need a target location this SpellEffect
// doesn't carry (only ever set from ForceSpell()/ForceSpellPoint(), which
// don't thread one through), so it's logged and dropped instead.
static bool
RunEffectPlay3DEffect(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL || effect.Resource().empty())
		return true;

	if (effect.Parameter2() == 2) {
		std::cerr << Log::Red << target->Name()
				<< ": Graphics: Play 3D Effect on point not implemented"
				<< Log::Normal << std::endl;
		return true;
	}

	AreaRoom* area = actor->Area();
	if (area != NULL)
		area->AddEffect(new ::Effect(effect.Resource().c_str(), actor->Position()));

	return true; // one-shot: remove immediately once applied
}


// #174 "Spell Effect: Play Sound Effect" - plays the sound named by the
// effect's resource key (unlike #215, IESDP lists no Target/Type
// parameters for this one - just the resource). Core::PlaySound() is
// currently a stub (no audio backend wired up yet), so this is a no-op in
// practice until that's implemented, but it's the correct hook point.
static bool
RunEffectPlaySoundEffect(Object* target, SpellEffect& effect)
{
	if (!effect.Resource().empty())
		Core::Get()->PlaySound(effect.Resource().c_str());

	return true; // one-shot: remove immediately once applied
}


// #238 "Death: Disintegrate". IESDP: kills the target if it matches an
// IDS Entry/File and hit-dice qualifier (Parameter1/Parameter2). This is
// what the opening BG2 cutscene uses to kill Irenicus's spy
// (WIZARD_DISINTEGRATE2_IGNORE_RESISTANCE); the IDS/hit-dice qualifier
// check is skipped (unconditional kill) to match this codebase's existing
// no-resistances/no-saves scope for damage and death handling.
static bool
RunEffectDisintegrate(Object* target, SpellEffect& effect)
{
	Actor* actor = dynamic_cast<Actor*>(target);
	if (actor == NULL)
		return true;

	actor->ApplyDamage(actor->CRE()->CurrentHitPoints());

	return true; // one-shot: remove immediately once applied
}


static const EffectDescriptor kEffectsTable[] = {
	{ 0, "Stat: AC vs. Damage Type Modifier", RunEffectACBonus, CleanupEffectACBonus },
	{ 3, "State: Berserking", RunEffectBerserk, CleanupEffectBerserk },
	{ 4, "Cure: Berserking", RunEffectCureBerserk },
	{ 12, "HP: Damage", RunEffectHPDamage },
	{ 13, "Death: Instant Death", RunEffectInstantDeath },
	{ 16, "State: Haste", RunEffectHaste, CleanupEffectHaste },
	{ 20, "State: Invisibility", RunEffectInvisibility, CleanupEffectInvisibility },
	{ 24, "State: Horror", RunEffectPanic, CleanupEffectPanic },
	{ 25, "State: Poison", RunEffectPoison, CleanupEffectPoison },
	{ 38, "State: Silence", RunEffectSilence, CleanupEffectSilence },
	{ 39, "State: Unconsciousness", RunEffectSleep, CleanupEffectSleep },
	{ 40, "State: Slow", RunEffectSlow, CleanupEffectSlow },
	{ 45, "State: Stun", RunEffectStun, CleanupEffectStun },
	{ 54, "Stat: THAC0 Modifier", RunEffectTHAC0Bonus, CleanupEffectTHAC0Bonus },
	{ 74, "State: Blindness", RunEffectBlindness, CleanupEffectBlindness },
	{ 109, "State: Hold", RunEffectHold, CleanupEffectHold },
	{ 124, "Spell Effect: Teleport (Dimension Door)", RunEffectTeleportToTarget },
	{ 128, "State: Confusion", RunEffectConfusion, CleanupEffectConfusion },
	{ 174, "Spell Effect: Play Sound Effect", RunEffectPlaySoundEffect },
	{ 175, "State: Hold", RunEffectHold, CleanupEffectHold },
	{ 185, "State: Hold", RunEffectHold, CleanupEffectHold },
	{ 215, "Graphics: Play 3D Effect", RunEffectPlay3DEffect },
	{ 238, "Death: Disintegrate", RunEffectDisintegrate },
	{ 242, "Cure: Confusion", RunEffectCureConfusion },
	{ 274, "Teleport to Target", RunEffectTeleportToTarget },
	{ 2, "Cure: Sleep", RunEffectCureSleep },
	{ 162, "Cure: Hold", RunEffectCureHold },

	// Not implemented: IESDP gives 40 Parameter2 values (0-39), each a
	// hardcoded BAM to play, but only names them descriptively ("aqua
	// SHAIR", "blue SHEARTH", ...) rather than by resref. Only value 39
	// ("Finger of Death") has a literal resref (SPFDEATH), confirmed to
	// exist in this install's game data; the other 39 would have to be
	// guessed, and Effect (game/Effect.h) doesn't tolerate a missing
	// resource. Left unimplemented rather than risk playing the wrong
	// effect or crashing on one that doesn't exist.
	{ 141, "Graphics: Lighting Effects", NULL },
};


const EffectDescriptor*
GetEffectDescriptor(int16 opcode)
{
	static const std::unordered_map<int16, const EffectDescriptor*> sByOpcode = [] {
		std::unordered_map<int16, const EffectDescriptor*> map;
		for (const auto& descriptor : kEffectsTable)
			map.emplace(descriptor.opcode, &descriptor);
		return map;
	}();

	auto found = sByOpcode.find(opcode);
	return found != sByOpcode.end() ? found->second : NULL;
}

