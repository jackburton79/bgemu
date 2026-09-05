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


bool
ITMResource::GetAbility(uint16 index, itm_ability& ability) const
{
	uint32 extHeaderOffset;
	uint16 extHeaderCount;
	fData->ReadAt(0x0064, extHeaderOffset);
	fData->ReadAt(0x0068, extHeaderCount);

	if (index >= extHeaderCount)
		return false;

	const uint32 kAbilitySize = 56;
	const uint32 offset = extHeaderOffset + index * kAbilitySize;

	fData->ReadAt(offset + 0x00, ability.attackType);
	fData->ReadAt(offset + 0x0e, ability.range);
	fData->ReadAt(offset + 0x12, ability.speed);
	fData->ReadAt(offset + 0x14, ability.thac0Bonus);
	fData->ReadAt(offset + 0x16, ability.diceSides);
	fData->ReadAt(offset + 0x18, ability.diceThrown);
	fData->ReadAt(offset + 0x1a, ability.damageBonus);
	fData->ReadAt(offset + 0x1c, ability.damageType);

	return true;
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

	fData->ReadAt(8, fHeader);

	//std::cout << "Name: " << IDTable::ObjectAt(fHeader.name_identified) << std::endl;

	return true;
}
