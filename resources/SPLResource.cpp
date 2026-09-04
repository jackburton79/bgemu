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
SPLResource::CastingTime() const
{
	// TODO: There could me multiple extended headers
	// we only check the first
	uint16 castingTime; // in tenth of round
	fData->ReadAt(fExtendedHeadersOffset + 0x0012, castingTime);
	return castingTime;
}


std::vector<spl_effect>
SPLResource::Effects() const
{
	// Prefer the first extended header's own feature blocks: this is
	// where single-ability spells (most innate/special abilities, which
	// is what ForceSpell()/ForceSpellPoint() force-cast) keep their real
	// effects. Fall back to the spell's top-level "casting" feature
	// blocks if there's no extended header, or it carries none.
	if (fExtendedHeadersCount > 0) {
		uint16 count;
		uint16 index;
		// Extended Header offsets 0x1e (count) and 0x20 (index) - see
		// IESDP spl_v1.
		fData->ReadAt(fExtendedHeadersOffset + 0x001e, count);
		fData->ReadAt(fExtendedHeadersOffset + 0x0020, index);
		if (count > 0)
			return _ReadFeatureBlocks(index, count);
	}

	return _ReadFeatureBlocks(fCastingFeatureBlockIndex, fCastingFeatureBlockCount);
}


std::vector<spl_effect>
SPLResource::_ReadFeatureBlocks(uint32 index, uint16 count) const
{
	const uint32 kFeatureBlockSize = 48;

	std::vector<spl_effect> effects;
	for (uint16 i = 0; i < count; i++) {
		uint32 blockOffset = fFeatureBlockOffset + (index + i) * kFeatureBlockSize;

		spl_effect effect;
		fData->ReadAt(blockOffset + 0x00, effect.opcode);
		fData->ReadAt(blockOffset + 0x02, effect.targetType);
		fData->ReadAt(blockOffset + 0x03, effect.power);
		fData->ReadAt(blockOffset + 0x04, effect.parameter1);
		fData->ReadAt(blockOffset + 0x08, effect.parameter2);
		fData->ReadAt(blockOffset + 0x0c, effect.timingMode);
		fData->ReadAt(blockOffset + 0x0e, effect.duration);
		fData->ReadAt(blockOffset + 0x14, effect.resource);

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
