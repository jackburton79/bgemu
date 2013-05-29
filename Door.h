#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"
#include "Object.h"
#include "Polygon.h"

#include <map>
#include <vector>

class Door : public Object {
public:
	Door(IE::door* areaDoor);
	void Toggle();

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	const Polygon& OpenPolygon() const;
	const Polygon& ClosedPolygon() const;
	void Print() const;

	static void Add(Door* door);
	static Door* GetByName(const char* name);
	static std::map<std::string, Door*>& List();

private:
	IE::door* fAreaDoor;
	Polygon fOpenPolygon;
	Polygon fClosedPolygon;

	static std::map<std::string, Door*> sDoors;
};

#endif // __DOOR_H
