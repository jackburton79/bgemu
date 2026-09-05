/*
 * SPLResource.cpp
 *
 *  Created on: 12 giu 2020
 *      Author: Stefano Ceccherini
 */

#include "SPLResource.h"


#define SPL_SIGNATURE "SPL "
#define SPL_VERSION_1 "V1  "

#include <Stream.h>

/* static */
Resource*
SPLResource::Create(const res_ref& name)
{
	return new SPLResource(name);
}


SPLResource::SPLResource(const res_ref& name)
	:
	Resource(name, RES_SPL)
{
}


bool
SPLResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(SPL_SIGNATURE) ||
		!CheckVersion(SPL_VERSION_1))
		return false;

	fData->ReadAt(0x0064, fExtendedHeadersOffset);
	fData->ReadAt(0x0068, fExtendedHeadersCount);
	fData->ReadAt(0x006a, fFeatureBlockOffset);
	fData->ReadAt(0x006e, fCastingFeatureBlockIndex);
	fData->ReadAt(0x0070, fCastingFeatureBlockCount);

	return true;
}


uint32
SPLResource::NameUnidentifiedRef() const
{
	uint32 strRef;
	fData->ReadAt(8, strRef);
	return strRef;
}


uint32
SPLResource::NameIdentifiedRef() const
{
	uint32 strRef;
	fData->ReadAt(12, strRef);
	return strRef;
}


uint32
SPLResource::Flags() const
{
	uint32 flags;
	fData->ReadAt(24, flags);
	return flags;
}


uint16
SPLResource::CastingGraphics() const
{
	uint16 id;
	fData->ReadAt(34, id);
	return id;
}


uint32
SPLResource::DescriptionUnidentifiedRef() const
{
	uint32 strRef;
	fData->ReadAt(80, strRef);
	return strRef;
}


uint32
SPLResource::DescriptionIdentifiedRef() const
{
	uint32 strRef;
	fData->ReadAt(84, strRef);
	return strRef;
}


uint16
SPLResource::CastingTime(uint16 abilityIndex) const
{
	uint16 castingTime = 0; // in tenth of round
	if (abilityIndex < fExtendedHeadersCount) {
		const uint32 kExtHeaderSize = 40;
		fData->ReadAt(fExtendedHeadersOffset + abilityIndex * kExtHeaderSize + 0x0012,
			castingTime);
	}
	return castingTime;
}


std::vector<spl_effect>
SPLResource::Effects(uint16 abilityIndex) const
{
	// Prefer the selected extended header's own feature blocks: this is
	// where single-ability spells (most innate/special abilities, which
	// is what ForceSpell()/ForceSpellPoint() force-cast) keep their real
	// effects. Fall back to the spell's top-level "casting" feature
	// blocks if there's no such extended header, or it carries none.
	if (abilityIndex < fExtendedHeadersCount) {
		const uint32 kExtHeaderSize = 40;
		const uint32 extHeaderOffset = fExtendedHeadersOffset + abilityIndex * kExtHeaderSize;

		uint16 count;
		uint16 index;
		// Extended Header offsets 0x1e (count) and 0x20 (index) - see
		// IESDP spl_v1.
		fData->ReadAt(extHeaderOffset + 0x001e, count);
		fData->ReadAt(extHeaderOffset + 0x0020, index);
		if (count > 0)
			return ReadFeatureBlocks(fData, fFeatureBlockOffset, index, count);
	}

	return ReadFeatureBlocks(fData, fFeatureBlockOffset, fCastingFeatureBlockIndex,
		fCastingFeatureBlockCount);
}


std::vector<spl_effect>
ReadFeatureBlocks(Stream* data, uint32 baseOffset, uint32 index, uint16 count)
{
	const uint32 kFeatureBlockSize = 48;

	std::vector<spl_effect> effects;
	for (uint16 i = 0; i < count; i++) {
		uint32 blockOffset = baseOffset + (index + i) * kFeatureBlockSize;

		spl_effect effect;
		data->ReadAt(blockOffset + 0x00, effect.opcode);
		data->ReadAt(blockOffset + 0x02, effect.targetType);
		data->ReadAt(blockOffset + 0x03, effect.power);
		data->ReadAt(blockOffset + 0x04, effect.parameter1);
		data->ReadAt(blockOffset + 0x08, effect.parameter2);
		data->ReadAt(blockOffset + 0x0c, effect.timingMode);
		data->ReadAt(blockOffset + 0x0e, effect.duration);
		data->ReadAt(blockOffset + 0x12, effect.probability1);
		data->ReadAt(blockOffset + 0x13, effect.probability2);
		data->ReadAt(blockOffset + 0x14, effect.resource);
		data->ReadAt(blockOffset + 0x1c, effect.diceThrown);
		data->ReadAt(blockOffset + 0x20, effect.diceSides);
		data->ReadAt(blockOffset + 0x24, effect.savingThrowType);
		data->ReadAt(blockOffset + 0x28, effect.savingThrowBonus);

		effects.push_back(effect);
	}

	return effects;
}


/* static */
std::string
SPLResource::GetSpellResourceName(uint16 id)
{
	char stringID[16];
	snprintf(stringID, sizeof(stringID), "%u", id);
	std::string resourceName;
	switch (stringID[0]) {
		case '1':
			resourceName = "SPPR";
			break;
		case '2':
			resourceName = "SPWI";
			break;
		case '3':
			resourceName = "SPIN";
			break;
		case '4':
			resourceName = "SPCL";
			break;
		default:
			throw std::runtime_error("SPLResource::GetSpellResourceName(): wrong spell ID!");
			break;
	}

	resourceName.append(stringID + 1);
	return resourceName;
}
