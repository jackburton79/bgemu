/*
 * SPLResource.cpp
 *
 *  Created on: 12 giu 2020
 *      Author: Stefano Ceccherini
 */

#include "SPLResource.h"


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
