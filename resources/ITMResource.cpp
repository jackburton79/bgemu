/*
 * ITMResource.cpp
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */


#include "ITMResource.h"

#include "ResManager.h"
#include "Stream.h"

#define ITM_SIGNATURE "ITM "
#define ITM_VERSION_1 "V1  "


/* static */
Resource*
ITMResource::Create(const res_ref& name)
{
	return new ITMResource(name);
}


ITMResource::ITMResource(const res_ref& resName)
	:
	Resource(resName, RES_ITM)
{
}


ITMResource::~ITMResource()
{
}


std::string
ITMResource::Animation() const
{
	std::string animation;
	animation.push_back(fHeader.animation[0]);
	animation.push_back(fHeader.animation[1]);
	return animation;
}


uint16
ITMResource::Type() const
{
	return fHeader.type;
}


uint32
ITMResource::DescriptionRef() const
{
	uint32 ref;
	fData->ReadAt(0x0054, ref);
	return ref;
}


uint32
ITMResource::Weight() const
{
	return fHeader.weight;
}


uint32
ITMResource::Price() const
{
	return fHeader.price;
}


uint16
ITMResource::StackAmount() const
{
	return fHeader.stack_amount;
}


bool
ITMResource::GetAbility(uint16 index, itm_ability& ability) const
{
	if (index >= fExtHeaderCount)
		return false;

	const uint32 kAbilitySize = 56;
	const uint32 offset = fExtHeaderOffset + index * kAbilitySize;

	fData->ReadAt(offset + 0x00, ability.attackType);
	fData->ReadAt(offset + 0x0e, ability.range);
	fData->ReadAt(offset + 0x12, ability.speed);
	fData->ReadAt(offset + 0x14, ability.thac0Bonus);
	fData->ReadAt(offset + 0x16, ability.diceSides);
	fData->ReadAt(offset + 0x18, ability.diceThrown);
	fData->ReadAt(offset + 0x1a, ability.damageBonus);
	fData->ReadAt(offset + 0x1c, ability.damageType);
	fData->ReadAt(offset + 0x1e, ability.featureBlockCount);
	fData->ReadAt(offset + 0x20, ability.featureBlockIndex);

	return true;
}


std::vector<spl_effect>
ITMResource::OnHitEffects(uint16 abilityIndex) const
{
	itm_ability ability;
	if (!GetAbility(abilityIndex, ability) || ability.featureBlockCount == 0)
		return std::vector<spl_effect>();

	return ReadFeatureBlocks(fData, fFeatureBlockOffset, ability.featureBlockIndex,
		ability.featureBlockCount);
}


std::vector<spl_effect>
ITMResource::EquippingEffects() const
{
	if (fEquipFeatureBlockCount == 0)
		return std::vector<spl_effect>();

	return ReadFeatureBlocks(fData, fFeatureBlockOffset, fEquipFeatureBlockIndex,
		fEquipFeatureBlockCount);
}

/* virtual */
bool
ITMResource::Load(Archive *archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(ITM_SIGNATURE))
		return false;

	if (!CheckVersion(ITM_VERSION_1))
		return false;

	// Read fields individually (rather than one bulk struct blit like the
	// original code did) since the header has gaps this struct doesn't
	// model (the description/description-icon refs, already exposed via
	// DescriptionRef()) - a bulk read would silently misalign everything
	// after the first gap.
	fData->ReadAt(0x08, fHeader.name_unidentified);
	fData->ReadAt(0x0c, fHeader.name_identified);
	fData->ReadAt(0x10, fHeader.replacement_item);
	fData->ReadAt(0x18, fHeader.flags);
	fData->ReadAt(0x1c, fHeader.type);
	fData->ReadAt(0x1e, fHeader.usability);
	fData->ReadAt(0x22, fHeader.animation);
	fData->ReadAt(0x24, fHeader.min_level);
	fData->ReadAt(0x26, fHeader.min_strength);
	fData->ReadAt(0x28, fHeader.min_strength_bonus);
	fData->ReadAt(0x29, fHeader.kit_usability1);
	fData->ReadAt(0x2a, fHeader.min_intelligence);
	fData->ReadAt(0x2b, fHeader.kit_usability2);
	fData->ReadAt(0x2c, fHeader.min_dexterity);
	fData->ReadAt(0x2d, fHeader.kit_usability3);
	fData->ReadAt(0x2e, fHeader.min_wisdom);
	fData->ReadAt(0x2f, fHeader.kit_usability4);
	fData->ReadAt(0x30, fHeader.min_constitution);
	fData->ReadAt(0x31, fHeader.weapon_proficiency);
	fData->ReadAt(0x32, fHeader.min_charisma);
	fData->ReadAt(0x34, fHeader.price);
	fData->ReadAt(0x38, fHeader.stack_amount);
	fData->ReadAt(0x3a, fHeader.inventory_icon);
	fData->ReadAt(0x42, fHeader.lore_to_id);
	fData->ReadAt(0x44, fHeader.ground_icon);
	fData->ReadAt(0x4c, fHeader.weight);
	fData->ReadAt(0x60, fHeader.enchantment);

	fData->ReadAt(0x64, fExtHeaderOffset);
	fData->ReadAt(0x68, fExtHeaderCount);
	fData->ReadAt(0x6a, fFeatureBlockOffset);
	fData->ReadAt(0x6e, fEquipFeatureBlockIndex);
	fData->ReadAt(0x70, fEquipFeatureBlockCount);

	//std::cout << "Name: " << IDTable::ObjectAt(fHeader.name_identified) << std::endl;

	return true;
}
