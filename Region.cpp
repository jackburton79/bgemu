/*
 * Region.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "BCSResource.h"
#include "Region.h"
#include "ResManager.h"


Region::Region(IE::region* region)
	:
	Object(region->name),
	fRegion(region)
{
	BCSResource* scriptResource = gResManager->GetBCS(region->script);
	if (scriptResource != NULL) {
		Script* script = scriptResource->GetScript();
		if (script != NULL)
			SetScript(script);
	}

	gResManager->ReleaseResource(scriptResource);
}


Region::~Region()
{
}


uint16
Region::Type() const
{
	return fRegion->type;
}


IE::rect
Region::Frame() const
{
	return fRegion->rect;
}


IE::point
Region::Position() const
{
	IE::point point = { int16(fRegion->use_point_x),
						int16(fRegion->use_point_y) };
	return point;
}


res_ref
Region::DestinationArea() const
{
	return fRegion->destination;
}


const char*
Region::DestinationEntrance() const
{
	return fRegion->entrance_name;
}


const ::Polygon&
Region::Polygon() const
{
	return fPolygon;
}


uint32
Region::CursorIndex() const
{
	return fRegion->cursor_index;
}


int32
Region::InfoTextRef() const
{
	return fRegion->info_text;
}
