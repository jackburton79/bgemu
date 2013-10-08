/*
 * DLGResource.cpp
 *
 *  Created on: 04/ott/2013
 *      Author: stefano
 */

#include "DLGResource.h"

DLGResource::DLGResource(const res_ref& name)
	:
	Resource(name, RES_DLG)
{
}


DLGResource::~DLGResource()
{
}


/* virtual */
bool
DLGResource::Load(Archive* archive, uint32 key)
{
	return Resource::Load(archive, key);
}
