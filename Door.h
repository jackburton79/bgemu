#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"


#include <vector>

class Door : public Object {
public:
	Door(IE::door* areaDoor);

	// TODO: Remove!
	void Toggle();

	void Open(Object* actor);
	void Close(Object* actor);

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	
	IE::rect Frame() const;
	IE::rect OpenBox() const;
	IE::rect ClosedBox() const;
	
	const Polygon& OpenPolygon() const;
	const Polygon& ClosedPolygon() const;

	virtual IE::point NearestPoint(const IE::point& point) const;

	void Print() const;

private:
	IE::door* fAreaDoor;
	Polygon fOpenPolygon;
	Polygon fClosedPolygon;
};

#endif // __DOOR_H
