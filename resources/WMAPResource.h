/*
 * WMAPResource.h
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */

#ifndef __WMAPRESOURCE_H
#define __WMAPRESOURCE_H

#include "IETypes.h"
#include "Resource.h"

#include <vector>

struct worldmap_entry {
	res_ref background_mos;
	uint32 width;
	uint32 height;
	uint32 number;
	uint32 nameref;
	uint32 unk1;
	uint32 unk2;
	uint32 areaentries_count;
	uint32 areaentries_offset;
	uint32 arealinkentries_offset;
	uint32 arealinkentries_count;
	res_ref map_icons_bam;
	char u[128];
};


enum area_flags {
	AREA_VISIBLE 				= 1 << 0,
	AREA_VISIBLE_FROM_ADJACENT 	= 1 << 1,
	AREA_REACHABLE 				= 1 << 2,
	AREA_VISITED				= 1 << 3
};

struct area_entry {
	res_ref area;
	res_ref shortname;
	char name[32];
	uint32 flags;
	uint32 icons_bam_sequence;
	uint32 x;
	uint32 y;
	uint32 caption_ref;
	uint32 tooltip_ref;
	res_ref loading_mos;
	uint32 link_index_north;
	uint32 link_count_north;
	uint32 link_index_west;
	uint32 link_count_west;
	uint32 link_index_south;
	uint32 link_count_south;
	uint32 link_index_east;
	uint32 link_count_east;
	char u[128];
};


enum entryloc {
	ENTRY_NORTH = 0x1,
	ENTRY_EAST	= 0x2,
	ENTRY_SOUTH = 0x4,
	ENTRY_WEST	= 0x8
};


struct arealink_entry {
	uint32 destination_index;
	char entry_point[32];
	uint32 travel_time;
	uint32 entry_location;
	res_ref random_encounter_1;
	res_ref random_encounter_2;
	res_ref random_encounter_3;
	res_ref random_encounter_4;
	res_ref random_encounter_5;
	uint32 random_encounter_probability;
	char u[128];
};


namespace GFX {
	struct rect;
}

class AreaEntry;
class BAMResource;
class WMAPResource : public Resource {
public:
	WMAPResource(const res_ref &name);
	static Resource* Create(const res_ref& name);

	virtual bool Load(Archive* archive, uint32 key);

	worldmap_entry WorldMapEntry();
	uint32 CountAreaEntries() const;
	AreaEntry* GetAreaEntry(uint32 index);
private:
	virtual ~WMAPResource();
	uint32 fCount;
	uint32 fOffset;

	worldmap_entry fWorldMapEntry;
	BAMResource* fIcons;
};


class AreaEntry {
public:
	AreaEntry(const area_entry& entry);
	~AreaEntry();

	IE::point Position() const;
	GFX::rect Rect() const;
	const Bitmap* Icon() const;
	res_ref Name() const;
	const char* LongName() const;
	std::string Caption() const;
	std::string TooltipName() const;
	res_ref LoadingScreenName() const;

private:
	friend class WMAPResource;

	area_entry fEntry;
	IE::point fPosition;
	Bitmap* fIcon;
};

#endif /* __WMAPRESOURCE_H */
