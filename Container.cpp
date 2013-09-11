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


const IE::rect
Container::Frame() const
{
	IE::rect rect;
	rect.x_min = fContainer->x_min;
	rect.x_max = fContainer->x_max;
	rect.y_min = fContainer->y_min;
	rect.y_max = fContainer->y_max;
	return rect;
}
