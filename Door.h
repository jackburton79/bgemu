#ifndef __DOOR_H
#define __DOOR_H

#include "IETypes.h"

#include <vector>

class Door {
public:
	Door(door_wed *);
	void Toggle();

	std::vector<uint16> fTilesOpen;

	bool Opened() const;
private:
	door_wed fDoor;
	bool fOpen;
};

#endif // __DOOR_H
