/*
 * SpellEffect.cpp
 *
 *  Created on: 7 gen 2022
 *      Author: Jackburton
 */

#include "SpellEffect.h"

#include "Object.h"

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
// NOTE: this mirrors the *exact same* placeholder behavior the old
// name-based WIZARD_DIMENSION_DOOR hack had (teleport to the map origin) -
// it's routed through opcode-based dispatch now instead of a spell-name
// string match, but the actual destination logic (a real target location -
// e.g. a saved home point, or wherever "Target" resolves to for this
// specific opcode) is still a TODO. The exact parameter semantics of
// opcode #274 aren't confirmed against IESDP directly; this was pieced
// together from indirect references. Verify against a real .spl dump
// before trusting parameter1/parameter2 for anything beyond this.
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


static const EffectDescriptor kEffectsTable[] = {
	{ 274, "Teleport to Target", RunEffectTeleportToTarget },

	// Not yet implemented - Object::_ApplySpellEffects() will drop these
	// immediately (with a warning) rather than leave them stuck. Adding a
	// real handler requires knowing the Actor/CRE API for applying HP
	// damage and state flags (silence/stun/blind/invisible), which hasn't
	// been reviewed yet.
	{ 12, "HP: Damage", NULL },
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

