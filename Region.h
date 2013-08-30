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
#include "Polygon.h"

class Region: public Object {
public:
	Region(IE::region* region);
	virtual ~Region();

	uint16 Type() const;
	IE::rect Frame() const;

	res_ref DestinationArea() const;
	const char* DestinationEntrance() const;

	const ::Polygon& Polygon() const;
private:
	IE::region* fRegion;
	::Polygon fPolygon;
};

#endif /* REGION_H_ */
