/*
 * ITMResource.cpp
 *
 *  Created on: 25/mag/2013
 *      Author: stefano
 */

#include "ResManager.h"
#include "ITMResource.h"
#include "MemoryStream.h"

#define ITM_SIGNATURE "ITM "
#define ITM_VERSION_1 "V1  "



ITMResource::ITMResource(const res_ref& resName)
	:
	Resource(resName, RES_ITM)
{
}


ITMResource::~ITMResource()
{
}


uint16
ITMResource::Type() const
{
	return fHeader.type;
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

	std::cout << "Name: " << IDTable::ObjectAt(fHeader.name_identified) << std::endl;

	return true;
}
