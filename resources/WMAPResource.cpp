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
#include "TLKResource.h"
#include "WMAPResource.h"

#include <memory>

#define WMAP_SIGNATURE "WMAP"
#define WMAP_VERSION_1 "V1.0"

WMAPResource::WMAPResource(const res_ref &name)
	:
	Resource(name, RES_WMP),
	fCount(0),
	fOffset(0),
	fIcons(NULL)
{

}


WMAPResource::~WMAPResource()
{
	gResManager->ReleaseResource(fIcons);
	std::vector<AreaEntry*>::const_iterator i;
	for (i = fAreaEntries.begin(); i != fAreaEntries.end(); i++) {
		delete *i;
	}
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
			fData->ReadAt(
					fWorldMapEntry.areaentries_offset
					+ c * sizeof(area_entry),
					areaEntry);
			AreaEntry* entry = new AreaEntry(areaEntry);
			std::cout << "Area " << areaEntry.area;
			std::cout << ", short: " << areaEntry.shortname;
			std::cout << ", long: " <<  areaEntry.name;
			std::cout << ", tooltip: " << areaEntry.tooltip_ref;
			std::cout << ", loading " << areaEntry.loading_mos;
			std::cout << std::endl;


			entry->fIcon = const_cast<Bitmap*>(fIcons->FrameForCycle(areaEntry.icons_bam_sequence, 0));
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

	fData->ReadAt(fWorldMapEntry.areaentries_offset
			+ index * sizeof(area_entry), area);
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
AreaEntry::AreaEntry(const area_entry& entry)
	:
	fEntry(entry)
{
	fIcon = NULL;
}


AreaEntry::~AreaEntry()
{
}


res_ref
AreaEntry::Name() const
{
	return fEntry.area;
}


const char*
AreaEntry::LongName() const
{
	return fEntry.name;
}


res_ref
AreaEntry::LoadingScreenName() const
{
	printf("loading screen %s\n", fEntry.loading_mos.CString());
	return fEntry.loading_mos;
}


IE::point
AreaEntry::Position() const
{
	return fPosition;
}


GFX::rect
AreaEntry::Rect() const
{
	GFX::rect rect((int16)fEntry.x - fIcon->Width() / 2,
				(int16)fEntry.y - fIcon->Height() / 2,
				fIcon->Width(), fIcon->Height());
	return rect;
}


const Bitmap*
AreaEntry::Icon() const
{
	return fIcon;
}


char*
AreaEntry::TooltipName() const
{
	TLKEntry* entry = Dialogs()->EntryAt(fEntry.tooltip_ref);
	if (entry == NULL)
		return NULL;

	std::cout << "tooltip: " << entry->string << std::endl;
	std::auto_ptr<TLKEntry> _(entry);

	return strdup(entry->string);
}
