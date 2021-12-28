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


/* static */
std::string
SPLResource::GetSpellResourceName(uint16 id)
{
	char stringID[5];
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
