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

	return true;
}


uint32
SPLResource::SpellNameUnidentified() const
{
	uint32 strRef;
	fData->ReadAt(8, strRef);
	return strRef;
}


uint32
SPLResource::SpellNameIdentified() const
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
SPLResource::SpellDescriptionUnidentified() const
{
	uint32 strRef;
	fData->ReadAt(80, strRef);
	return strRef;
}


uint32
SPLResource::SpellDescriptionIdentified() const
{
	uint32 strRef;
	fData->ReadAt(84, strRef);
	return strRef;
}
