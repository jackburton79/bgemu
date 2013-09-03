/*
 * Container.cpp
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#include "Container.h"

Container::Container(IE::container* container)
	:
	Object(container->name),
	fContainer(container)
{
}


Container::~Container()
{

}


const ::Polygon&
Container::Polygon() const
{
	return fPolygon;
}
