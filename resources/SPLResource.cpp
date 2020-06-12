/*
 * SPLResource.cpp
 *
 *  Created on: 12 giu 2020
 *      Author: Stefano Ceccherini
 */

#include "SPLResource.h"


#define SPL_SIGNATURE "SPL "
#define SPL_VERSION_1 "V1  "


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
