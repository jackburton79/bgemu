#include "Door.h"

Door::Door(door* areaDoor)
	:
	fAreaDoor(areaDoor)
{
}


void
Door::Toggle()
{
	//printf("door %s toggled\n", fDoor.name);

	bool wasOpen = fAreaDoor->flags & DOOR_OPEN;

	if (wasOpen)
		fAreaDoor->flags &= ~DOOR_OPEN;
	else
		fAreaDoor->flags |= DOOR_OPEN;
}


bool
Door::Opened() const
{
	return fAreaDoor->flags & DOOR_OPEN;
}


void
Door::Print() const
{
	printf("%s\n", (const char*)fAreaDoor->id);
}
