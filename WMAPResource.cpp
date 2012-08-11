/*
 * WMAPResource.cpp
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */


#include "BamResource.h"
#include "GraphicsEngine.h"
#include "MemoryStream.h"
#include "ResManager.h"
#include "WMAPResource.h"

#define WMAP_SIGNATURE "WMAP"
#define WMAP_VERSION_1 "V1.0"

WMAPResource::WMAPResource(const res_ref &name)
	:
	Resource(name, RES_WMP),
	fIcons(NULL)
{

}


WMAPResource::~WMAPResource()
{
	gResManager->ReleaseResource(fIcons);
	fAreaEntries.erase(fAreaEntries.begin(), fAreaEntries.end());
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

	fIcons = gResManager->GetBAM(fWorldMapEntry.map_icons_bam);

	for (uint32 c = 0; c < fWorldMapEntry.areaentries_count; c++) {
		area_entry areaEntry;
		if (GetAreaEntry(c, areaEntry)) {
			fData->ReadAt(fWorldMapEntry.areaentries_offset + c * sizeof(area_entry), areaEntry);
			AreaEntry* entry = new AreaEntry(this);
			entry->fIcon = fIcons->FrameForCycle(areaEntry.icons_bam_sequence, 0);
			strcpy(entry->fName, areaEntry.area);
			entry->fPosition.x = (int16)areaEntry.x;
			entry->fPosition.y = (int16)areaEntry.y;
			fAreaEntries.push_back(entry);

		}
	}

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


AreaEntry&
WMAPResource::AreaEntryAt(uint32 index)
{
	if (index >= fWorldMapEntry.areaentries_count)
		throw -1;

	return *fAreaEntries[index];
}


// AreaEntry
AreaEntry::AreaEntry(WMAPResource*)
{

}


AreaEntry::~AreaEntry()
{
	GraphicsEngine::DeleteBitmap(fIcon.bitmap);
}


const char*
AreaEntry::Name() const
{
	return fName;
}


IE::point
AreaEntry::Position() const
{
	return fPosition;
}


GFX::rect
AreaEntry::Rect() const
{
	GFX::rect rect = {
			fPosition.x - fIcon.rect.w / 2,
			fPosition.y - fIcon.rect.h / 2,
			fIcon.rect.w, fIcon.rect.h
	};
	return rect;
}


Frame
AreaEntry::Icon() const
{
	return fIcon;
}
