/*
 * Region.cpp
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */

#include "BCSResource.h"
#include "Region.h"
#include "ResManager.h"

std::vector<Region*> Region::sRegions;

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


/* static */
void
Region::Add(Region* region)
{
	sRegions.push_back(region);
}


/* static */
std::vector<Region*>&
Region::List()
{
	return sRegions;
}
