#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"

#include <vector>

class Door {
public:
	Door(door* areaDoor);
	void Toggle();

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
	void Print() const;
private:
	door* fAreaDoor;
};

#endif // __DOOR_H
