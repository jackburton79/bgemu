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

// One "Feature Block" entry (48 bytes in the .spl file) - a single effect
// carried by the spell. See IESDP spl_v1 for the authoritative layout.
struct spl_effect {
	int16 opcode;
	uint8 targetType;
	uint8 power;
	int32 parameter1;
	int32 parameter2;
	uint8 timingMode;
	uint32 duration;
	res_ref resource;
};

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

	uint16 CastingTime() const;

	// Effects this spell applies when it hits its target. Prefers the
	// first extended header's feature blocks (where single-ability spells -
	// e.g. innate/special abilities force-cast via ForceSpell() - keep
	// their actual effects); falls back to the spell's top-level "casting"
	// feature blocks (applied regardless of ability) if there's none.
	std::vector<spl_effect> Effects() const;

	static std::string GetSpellResourceName(uint16 id);

private:
	std::vector<spl_effect> _ReadFeatureBlocks(uint32 index, uint16 count) const;

	uint32 fExtendedHeadersOffset;
	uint16 fExtendedHeadersCount;

	uint32 fFeatureBlockOffset;
	uint16 fCastingFeatureBlockIndex;
	uint16 fCastingFeatureBlockCount;
};



#endif // SPLRESOURCE_H_
