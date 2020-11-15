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

#include <list>

class Region: public Object {
public:
	Region(IE::region* region);
	virtual ~Region();

	uint16 Type() const;
	IE::rect Frame() const;
	virtual IE::point Position() const;

	bool Contains(IE::point) const;

	res_ref DestinationArea() const;
	const char* DestinationEntrance() const;

	const ::Polygon& Polygon() const;

	uint32 CursorIndex() const;
	int32 InfoTextRef() const;
	
	void ActivateTrigger();

	void ActorEntered(Actor* actor);
	void ActorExited(Actor* actor);
	bool IsActorInside(Actor* actor) const;
	bool IsActorInside(object_node* actorNode) const;


private:
	IE::region* fRegion;
	::Polygon fPolygon;

	std::list<Actor*> fObjectsInside;
};

#endif /* REGION_H_ */
