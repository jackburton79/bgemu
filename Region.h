/*
 * Region.h
 *
 *  Created on: 20/mag/2013
 *      Author: stefano
 */


#ifndef __REGION_H_
#define __REGION_H_

#include "IETypes.h"
#include "Object.h"

#include <vector>

class Region: public Object {
public:
	Region(IE::region* region);
	virtual ~Region();

	// Global list of regions
	static void Add(Region *region);
	//static void Remove(const char* name);
	static std::vector<Region*>& List();
private:
	IE::region* fRegion;

	static std::vector<Region*> sRegions;
};

#endif /* REGION_H_ */
