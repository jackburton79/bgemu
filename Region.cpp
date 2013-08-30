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
	if (scriptResource != NULL)
		SetScript(scriptResource->GetScript());
	gResManager->ReleaseResource(scriptResource);
}


Region::~Region()
{
}


IE::rect
Region::Frame() const
{
	return fRegion->rect;
}


res_ref
Region::DestinationArea() const
{
	return fRegion->destination;
}
