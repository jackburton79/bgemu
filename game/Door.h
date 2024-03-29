#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"


#include <vector>


class Door : public Object {
public:
	Door(IE::door* areaDoor);

	res_ref ShortName() const;	
	
	// TODO: Remove!
	void Toggle();

	void Open(Object* object);
	void Close(Object* object);

	void Lock();
	void Unlock();

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	
	virtual IE::point NearestPoint(const IE::point& start) const;
	
	virtual IE::rect Frame() const;
	IE::rect OpenBox() const;
	IE::rect ClosedBox() const;
	
	virtual ::Outline Outline() const;

	const Polygon& OpenPolygon() const;
	const Polygon& ClosedPolygon() const;

	void Print() const;

private:
	IE::door* fAreaDoor;
	IE::tiled_object* fTiledObject;	
	Polygon fOpenPolygon;
	Polygon fClosedPolygon;
};

#endif // __DOOR_H
