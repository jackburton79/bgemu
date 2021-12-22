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

/* static */
Resource*
WMAPResource::Create(const res_ref& name)
{
	return new WMAPResource(name);
}


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


AreaEntry*
WMAPResource::GetAreaEntry(uint32 index)
{
	if (index >= fWorldMapEntry.areaentries_count)
		return NULL;

	area_entry areaEntry;
    fData->ReadAt(
    		fWorldMapEntry.areaentries_offset
				+ index * sizeof(area_entry), areaEntry);
    AreaEntry* entry = new AreaEntry(areaEntry);
    entry->fIcon = fIcons->FrameForCycle(areaEntry.icons_bam_sequence, 0);
    entry->fPosition.x = (int16)areaEntry.x;
    entry->fPosition.y = (int16)areaEntry.y;
    return entry;
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
	fIcon->Release();
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


std::string
AreaEntry::Caption() const
{
	TLKEntry* entry = Dialogs()->EntryAt(fEntry.caption_ref);
	if (entry == NULL)
		return "";

	std::string toolTip = entry->text;

	delete entry;
	return toolTip;
}


std::string
AreaEntry::TooltipName() const
{
	TLKEntry* entry = Dialogs()->EntryAt(fEntry.tooltip_ref);
	if (entry == NULL)
		return "";

	std::string toolTip = entry->text;
	
	delete entry;
	return toolTip;
}
