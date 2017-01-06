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
	~Container();

	IE::rect Frame() const;
	const ::Polygon& Polygon() const;

private:
	IE::container* fContainer;
	::Polygon fPolygon;
};

#endif /* CONTAINER_H_ */
