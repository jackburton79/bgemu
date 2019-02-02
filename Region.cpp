/*
 * Region.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "Region.h"

#include "BCSResource.h"
#include "Core.h"
#include "ResManager.h"

#include <algorithm>

Region::Region(IE::region* region)
	:
	Object(region->name, region->script.CString()),
	fRegion(region)
{
	// TODO: Region names contain spaces, but scripts search
	// for them without spaces, so we remove them
	std::string fixedName = region->name;
	fixedName.erase(std::remove_if(fixedName.begin(), fixedName.end(), isspace), fixedName.end());
	SetName(fixedName.c_str());
	if (fixedName.compare("Rielevdeadtrigger") == 0) {
		region->Print();
	}
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


bool
Region::Contains(IE::point point) const
{
	return fPolygon.Contains(point.x, point.y);
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


void
Region::ActivateTrigger()
{
}


void
Region::CheckObjectsInside()
{
	std::list<Object*>::iterator i = fObjectsInside.begin();
	while (i != fObjectsInside.end()) {
		Object* object = (*i);
		if (!Contains(object->Position())) {
			object->ExitedRegion(this);
			i = fObjectsInside.erase(i);
		} else
			i++;
	}

	Object* object = Core::Get()->GetObject(this);
	if (object == NULL)
		return;
	i = std::find(fObjectsInside.begin(),
					fObjectsInside.end(), object);
	if (i == fObjectsInside.end()) {
		fObjectsInside.push_back(object);
		object->EnteredRegion(this);
	}
}
