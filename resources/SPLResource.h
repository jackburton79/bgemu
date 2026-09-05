/*
 * SPLResource.h
 *
 *  Created on: 12 giu 2020
 *      Author: Stefano Ceccherini
 */

#ifndef SPLRESOURCE_H_
#define SPLRESOURCE_H_

#include "Resource.h"

#include <vector>

// One "Feature Block" entry (48 bytes in the .spl/.itm file) - a single
// effect carried by the spell/item ability. See IESDP spl_v1/itm_v1 for
// the authoritative layout - both formats share this exact 48-byte
// structure, so ITMResource reuses this type instead of duplicating it.
struct spl_effect {
	int16 opcode;
	uint8 targetType;
	uint8 power;
	int32 parameter1;
	int32 parameter2;
	uint8 timingMode;
	uint32 duration;
	uint8 probability1;
	uint8 probability2;
	res_ref resource;
	int32 diceThrown;
	int32 diceSides;
	uint32 savingThrowType;	// bitmask: bit0 Spells, bit1 Breath, bit2 Death, bit3 Wands, bit4 Polymorph
	int32 savingThrowBonus;
};


// Shared by SPLResource and ITMResource: reads `count` consecutive 48-byte
// feature blocks starting at the block whose index (not byte offset) is
// `index`, in the feature-block data segment starting at `baseOffset`.
std::vector<spl_effect> ReadFeatureBlocks(Stream* data, uint32 baseOffset,
	uint32 index, uint16 count);


class SPLResource : public Resource {
public:
	static Resource* Create(const res_ref& name);

	SPLResource(const res_ref &name);

	virtual bool Load(Archive *archive, uint32 key);

	uint32 NameUnidentifiedRef() const;
	uint32 NameIdentifiedRef() const;

	uint32 Flags() const;

	uint16 CastingGraphics() const;

	uint32 DescriptionUnidentifiedRef() const;
	uint32 DescriptionIdentifiedRef() const;

	// abilityIndex selects which Extended Header (ability) to read; spells
	// can have more than one (e.g. wands with different charge types).
	// Defaults to 0 to match every current caller (ForceSpell()/
	// ForceSpellPoint(), which always force-cast the spell's first/only
	// relevant ability).
	uint16 CastingTime(uint16 abilityIndex = 0) const;

	// Effects this spell applies when it hits its target. Prefers the
	// selected extended header's feature blocks (where single-ability
	// spells - e.g. innate/special abilities force-cast via ForceSpell() -
	// keep their actual effects); falls back to the spell's top-level
	// "casting" feature blocks (applied regardless of ability) if there's
	// none, or if `abilityIndex` is out of range.
	std::vector<spl_effect> Effects(uint16 abilityIndex = 0) const;

	static std::string GetSpellResourceName(uint16 id);

private:
	uint32 fExtendedHeadersOffset;
	uint16 fExtendedHeadersCount;

	uint32 fFeatureBlockOffset;
	uint16 fCastingFeatureBlockIndex;
	uint16 fCastingFeatureBlockCount;
};



#endif // SPLRESOURCE_H_
