/*
 * SpellEffect.cpp
 *
 *  Created on: 7 gen 2022
 *      Author: Jackburton
 */

#include "SpellEffect.h"

#include "Actor.h"
#include "AreaRoom.h"
#include "Effect.h"
#include "Log.h"
#include "Object.h"

#include <iostream>
#include <unordered_map>


SpellEffect::SpellEffect(int16 opcode, Object* source, int32 parameter1,
		int32 parameter2, uint32 duration, const std::string& resource)
	:
	fOpcode(opcode),
	fSource(source),
	fParameter1(parameter1),
	fParameter2(parameter2),
	fDuration(duration),
	fPermanent(duration == 0),
	fInitiated(false),
	fResource(resource)
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



// #274 "Teleport to Target"
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


static const EffectDescriptor kEffectsTable[] = {
	{ 12, "HP: Damage", RunEffectHPDamage },
	{ 215, "Graphics: Play 3D Effect", RunEffectPlay3DEffect },
	{ 274, "Teleport to Target", RunEffectTeleportToTarget },

	// Not yet implemented - Object::_ApplySpellEffects() will drop these
	// immediately (with a warning) rather than leave them stuck. Adding a
	// real handler requires knowing the Actor/CRE API for applying HP
	// damage and state flags (silence/stun/blind/invisible), which hasn't
	// been reviewed yet.
	{ 20, "State: Invisibility", NULL },
	{ 38, "State: Silence", NULL },
	{ 45, "State: Stun", NULL },
	{ 74, "State: Blindness", NULL },
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

