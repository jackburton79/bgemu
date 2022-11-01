/*
 * Container.h
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#ifndef CONTAINER_H_
#define CONTAINER_H_

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"

class Container: public Object {
public:
	Container(IE::container* container);

	virtual IE::rect Frame() const;
	::Outline Outline() const;
	
	const ::Polygon& Polygon() const;

private:
	virtual ~Container();
	IE::container* fContainer;
	::Polygon fPolygon;
};

#endif /* CONTAINER_H_ */
