/*
 * WMAPResource.cpp
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */


#include "MemoryStream.h"
#include "WMAPResource.h"

#define WMAP_SIGNATURE "WMAP"
#define WMAP_VERSION_1 "V1.0"

WMAPResource::WMAPResource(const res_ref &name)
	:
	Resource(name, RES_WMP)
{
	// TODO Auto-generated constructor stub

}

WMAPResource::~WMAPResource()
{
	// TODO Auto-generated destructor stub
}


/* virtual */
bool
WMAPResource::Load(Archive* archive, uint32 key)
{
	if (!Resource::Load(archive, key))
		return false;

	if (!CheckSignature(WMAP_SIGNATURE))
		return false;

	if (!CheckVersion(WMAP_VERSION_1))
		return false;

	fData->ReadAt(8, fCount);
	fData->ReadAt(12, fOffset);

	return true;
}


bool
WMAPResource::GetWorldMap(worldmap_entry& entry)
{
	fData->ReadAt(fOffset, entry);
	return true;
}
