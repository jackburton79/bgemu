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

}


WMAPResource::~WMAPResource()
{
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

	// TODO: Handle the case where there are more than one.
	fData->ReadAt(fOffset, fWorldMapEntry);
	return true;
}


worldmap_entry
WMAPResource::WorldMapEntry()
{
	return fWorldMapEntry;
}


uint32
WMAPResource::CountAreaEntries() const
{
	return fWorldMapEntry.areaentries_count;
}


bool
WMAPResource::GetAreaEntry(uint32 index, area_entry& area)
{
	if (index >= fWorldMapEntry.areaentries_count)
		return false;
	fData->ReadAt(fWorldMapEntry.areaentries_offset + index * sizeof(area_entry), area);
	return true;
}
