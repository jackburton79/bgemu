/*
 * WMAPResource.h
 *
 *  Created on: 18/lug/2012
 *      Author: stefano
 */

#ifndef __WMAPRESOURCE_H
#define __WMAPRESOURCE_H

#include "Resource.h"

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


struct area_entry {

};


struct arealink_entry {

};


class WMAPResource : public Resource {
public:
	WMAPResource(const res_ref &name);
	virtual ~WMAPResource();
	virtual bool Load(Archive* archive, uint32 key);

	bool GetWorldMap(worldmap_entry& entry);

private:
	uint32 fCount;
	uint32 fOffset;
};

#endif /* WMAPRESOURCE_H_ */
