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

	// CONTAINERENABLE(O:Object,I:Bool*BOOLEAN) - bit 5 of the ARE
	// container flags ("Disabled" per are_v1.htm), same raw-flags
	// approach Door already uses for its own lock/trap state.
	bool IsEnabled() const;
	void SetEnabled(bool enabled);

private:
	virtual ~Container();
	IE::container* fContainer;
	::Polygon fPolygon;
};

#endif /* CONTAINER_H_ */
