/*
 * ITMResource.cpp
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#include "ITMResource.h"

ITMResource::ITMResource(const res_ref& resName)
	:
	Resource(resName, RES_ITM)
{

}


ITMResource::~ITMResource()
{

}


/* virtual */
bool
ITMResource::Load(Archive *archive, uint32 key)
{
	return Resource::Load(archive, key);
}
