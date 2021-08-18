/*
 * Region.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "Region.h"

#include "Core.h"
#include "ResManager.h"

#include <algorithm>

Region::Region(IE::region* region)
	:
	Object(region->name, Object::REGION, region->script.CString()),
	fRegion(region)
{
	// TODO: Region names contain spaces, but scripts search
	// for them without spaces, so we remove them
	std::string fixedName = region->name;
	fixedName.erase(std::remove_if(fixedName.begin(), fixedName.end(), isspace), fixedName.end());
	SetName(fixedName.c_str());
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


::Outline
Region::Outline() const
{
	GFX::Color color;
	switch (Type()) {
		case IE::REGION_TYPE_TRAVEL:
			color = { 0, 125, 0 };
			break;
		case IE::REGION_TYPE_TRIGGER:
			color = { 225, 0, 0 };
			break;
		default:
			color = { 255, 255, 255 };
			break;
	}
	return ::Outline(fPolygon, color);
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
	if (fRegion->type != IE::REGION_TYPE_TRIGGER
		&& fRegion->type != IE::REGION_TYPE_INFO) {
		std::cerr << "ActivateTrigger() called on wrong region type: ";
		std::cerr << Name() <<  " (" << fRegion->type << ")" << std::endl;
		//return;
	}
	fRegion->Print();
}


void
Region::ActorEntered(Actor* actor)
{
	// TODO: check if is already in
	if (!IsActorInside(actor)) {
		fObjectsInside.push_back(actor);
		AddTrigger(trigger_entry("Entered", actor));
	}
}


void
Region::ActorExited(Actor* actor)
{
	for (std::list<Actor*>::iterator i = fObjectsInside.begin();
			i != fObjectsInside.end(); i++) {
		if ((*i)->GlobalID() == actor->GlobalID()) {
			fObjectsInside.remove(actor);
			break;
		}
	}
}


bool
Region::IsActorInside(Actor* actor) const
{
	for (std::list<Actor*>::const_iterator i = fObjectsInside.begin();
			i != fObjectsInside.end(); i++) {
		if ((*i)->GlobalID() == actor->GlobalID())
			return true;
	}
	return false;
}


bool
Region::IsActorInside(object_params* actorNode) const
{
	for (std::list<Actor*>::const_iterator i = fObjectsInside.begin();
			i != fObjectsInside.end(); i++) {
		if ((*i)->MatchNode(actorNode))
			return true;
	}
	return false;
}
