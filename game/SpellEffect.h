/*
 * SpellEffect.h
 *
 *  Created on: 7 gen 2022
 *      Author: Stefano Ceccherini
 */

#pragma once

#include "IETypes.h"

#include <string>

class Object;

// A single applied effect (one "Feature Block" out of a .spl file),
// attached to whichever Object it targets - see Object::AddSpellEffect().
//
// NOTE: duration/timing here is a first-pass simplification, not a full
// implementation of the IESDP "Timing mode" field (Instant/Delay,
// Limited/Permanent/While-equipped, etc.): an effect with duration == 0 is
// treated as permanent/one-shot (applied once, never auto-expires here);
// one with duration > 0 counts down in ticks and is removed once it
// reaches zero. Good enough for a first implementation, but not a
// faithful reproduction of every timing mode.
class SpellEffect {
public:
	SpellEffect(int16 opcode, Object* source, int32 parameter1,
		int32 parameter2, uint32 duration, const std::string& resource = "",
		uint32 savingThrowType = 0, int32 savingThrowBonus = 0);
	~SpellEffect();

	int16 Opcode() const;
	Object* Source() const;
	int32 Parameter1() const;
	int32 Parameter2() const;
	const std::string& Resource() const;

	// Saving throw type bitmask (bit0 Spells, bit1 Breath, bit2 Death,
	// bit3 Wands, bit4 Polymorph - see IESDP spl_v1 feature block) and
	// bonus, straight from the .spl file's feature block (spl_effect,
	// resources/SPLResource.h). 0 means "no save allowed".
	uint32 SavingThrowType() const;
	int32 SavingThrowBonus() const;

	// Whether the opcode handler has already run its one-time "apply"
	// logic (e.g. actually inflicting damage, or setting a state flag) -
	// separate from the tick-by-tick duration countdown.
	bool Initiated() const;
	void SetInitiated();

	// Decrements the remaining duration by one tick. Returns true once it
	// reaches zero (the effect should be removed). Permanent effects
	// (duration == 0 at construction) always return false here; it's up
	// to the opcode handler to remove them explicitly if it ever needs to.
	bool Tick();

	// For debug printing only.
	std::string Name() const;

private:
	int16 fOpcode;
	Object* fSource;
	int32 fParameter1;
	int32 fParameter2;
	uint32 fDuration;
	bool fPermanent;
	bool fInitiated;
	std::string fResource;
	uint32 fSavingThrowType;
	int32 fSavingThrowBonus;
};

// Applies/updates one tick of the effect on `target`.
// Returns true if the effect should be removed (expired, or a one-shot effect that has already run its course).
typedef bool (*EffectRunFunc)(Object* target, SpellEffect& effect);

// Called exactly once, right before a still-active effect is removed
// (duration expired) - the counterpart to `run` for effects that need to
// undo something they did (e.g. clear a STATE.IDS bit, subtract back an
// AC/THAC0 bonus). Not called for one-shot effects (those never make it
// past their first `run()`, which returns true immediately).
typedef void (*EffectCleanupFunc)(Object* target, SpellEffect& effect);

struct EffectDescriptor {
	int16 opcode;
	const char* name;

	// NULL for opcodes not yet implemented: Object::_ApplySpellEffects()
	// then drops the effect immediately (logging a warning) instead of
	// leaving it stuck on the object forever.
	EffectRunFunc run;

	// NULL if this effect needs no cleanup on expiry (the common case for
	// one-shot effects).
	EffectCleanupFunc cleanup = NULL;
};

// Returns NULL if opcode isn't a known/implemented effect.
const EffectDescriptor* GetEffectDescriptor(int16 opcode);

