/*
 * WorldMap.cpp
 *
 *  Created on: 19/lug/2012
 *      Author: stefano
 */

#include "Bitmap.h"
#include "ResManager.h"
#include "WMAPResource.h"
#include "WorldMap.h"


WorldMap::WorldMap()
{
	fWmap = gResManager->GetWMAP("WORLDMAP");

}

WorldMap::~WorldMap()
{
	gResManager->ReleaseResource(fWmap);
}

