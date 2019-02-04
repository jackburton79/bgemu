/*
 * Container.cpp
 *
 *  Created on: 03/set/2013
 *      Author: stefano
 */

#include "Container.h"

Container::Container(IE::container* container)
	:
	Object(container->name, container->trap_script.CString()),
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


IE::rect
Container::Frame() const
{
	IE::rect rect;
	rect.x_min = fContainer->x_min;
	rect.x_max = fContainer->x_max;
	rect.y_min = fContainer->y_min;
	rect.y_max = fContainer->y_max;
	return rect;
}


/* virtual */
IE::point
Container::Position() const
{
	// TODO: Not completely correct, take into account x_max and y_max
	IE::point point = {
		(int16)fContainer->x_min,
		(int16)fContainer->y_min
	};
	return point;
}
