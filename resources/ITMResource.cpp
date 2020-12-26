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
