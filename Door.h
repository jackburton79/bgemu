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

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	
	virtual IE::point NearestPoint(const IE::point& start) const;
	
	IE::rect Frame() const;
	IE::rect OpenBox() const;
	IE::rect ClosedBox() const;
	
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
